/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef MVCCTable_hpp_
#define MVCCTable_hpp_

#include <atomic>
#include <algorithm>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <random>
#include <unordered_map>
#include <vector>

#include "fmt/format.h"

#ifdef USE_ROCKSDB_TABLE
#include "table/RDBTable.hpp"
#elif USE_NVM_TABLES
#include "table/PBPTreeTable.hpp"
#else
#include "table/CuckooTable.hpp"
#include "table/HashMapTable.hpp"
#endif

#include "core/serialize.hpp"

#include "table/BaseTable.hpp"
#include "table/TableException.hpp"
#include "table/TableInfo.hpp"
#include "table/StateContext.hpp"

namespace pfabric {

/*******************************************************************************
 * @brief MVCC wrapper for a given RecordType
 ******************************************************************************/
template <typename RecordType, std::size_t VERSIONS = 2>
struct MVCCObject {
  static constexpr std::size_t Versions = VERSIONS;

  struct header {
    std::atomic<TransactionID> cts{DTS_INF};
    std::atomic<TransactionID> dts{0};
    header() noexcept : cts{DTS_INF}, dts{0} {}
    header(const header& rhs) noexcept : cts{rhs.cts.load()}, dts{rhs.dts.load()} {}
    header(header&& rhs) noexcept : cts{rhs.cts.load()}, dts{rhs.dts.load()} {}
    header& operator=(const header& rhs) {
      cts.store(rhs.cts.load());
      dts.store(rhs.dts.load());
      return *this;
    }
  };

  std::array<header, VERSIONS> headers;
  std::atomic_uint8_t usedSlots{0};
  std::array<RecordType, VERSIONS> values;
  // std::mutex mtx;

  /** Default Constructor */
  MVCCObject() noexcept : headers{}, usedSlots{0}, values{} {}

  /** Copy Constructor */
  MVCCObject(const MVCCObject& rhs) noexcept : headers{rhs.headers},
                                               usedSlots{rhs.usedSlots.load()},
                                               values{rhs.values} {}
  /** Move Constructor */
  MVCCObject(MVCCObject&& rhs) noexcept : headers{std::move(rhs.headers)},
                                          usedSlots{rhs.usedSlots.load()},
                                          values{std::move(rhs.values)} {}

  /** Copy Assignment Operator */
  MVCCObject& operator=(const MVCCObject& rhs) {
    usedSlots.store(rhs.usedSlots.load());
    headers = rhs.headers;
    values = rhs.values;
    return *this;
  }

  inline int getCurrent(const TransactionID& txnID) const {
    // const auto slots = usedSlots.load();
    for (auto i = 0u; i < VERSIONS; ++i) {
      if (usedSlots.load() & (1 << i)) {
        // const std::lock_guard<std::mutex> lock(mtx); //TODO: would need a NUMA suitable lock (PMDK?)
        if (headers[i].dts.load() > txnID &&
            headers[i].cts.load() <= txnID)
        return i;
      }
    }
    return -1;
  }

  inline void newEntry(const TransactionID& txnID, const TransactionID& iPos, const TransactionID& dPos,
                       const RecordType &value) {
    values[iPos] = std::move(value);
    headers[iPos].cts.store(txnID);
    headers[iPos].dts.store(DTS_INF);
    headers[dPos].dts.store(txnID);
#ifdef USE_NVM_TABLES
    /// Flush is necessary for failure-atomicity;
    // pmem_flush(&headers, sizeof(headers));
    // pmem_flush(&values[iPos], sizeof(RecordType));
    pmem_flush(this, sizeof(MVCCObject<RecordType>));
    pmem_drain();
#endif
    usedSlots.store(usedSlots.load() | (1LL << iPos));
#ifdef USE_NVM_TABLES
    pmem_flush(&usedSlots, sizeof(usedSlots));
    // pmem_drain();
#endif
  }

  inline void cleanUpVersions(const TransactionID& oldestReadVersion) {
    /// TODO: needs persistent guarantees?
    for(auto i = 0u; i < VERSIONS; ++i) {
      if (headers[i].dts.load() <= oldestReadVersion) {
        usedSlots.store(usedSlots.load() & ~(1 << i));
        headers[i].cts.store(DTS_INF);
        headers[i].dts.store(0);
      }
    }
  }
};

/*******************************************************************************
 * @brief Write set for collecting uncommitted operations
 ******************************************************************************/
template <typename KeyType, typename RecordType>
struct WriteSet {
  using Pair = std::pair<KeyType, RecordType>;
  using Set = std::vector<Pair>;

  TransactionID txnID;
  Set set;

  void insert(const KeyType& k, const RecordType& r) {
    set.emplace_back(k, r);
  }

  void insert(KeyType&& k, RecordType&& r) {
    set.emplace_back(std::move(k), std::move(r));
  }

  void clean() {
    txnID = 0;
    set.clear();
  }
};



/*******************************************************************************
 * @brief Shared lock for handling one writer + multiple readers
 ******************************************************************************/
template <typename KeyType>
class RWLocks {
  //cf. https://stackoverflow.com/a/28121513
  public:
    void lockShared(KeyType key) {
      auto &rl = locks[key];
      std::unique_lock<std::mutex> lk(rl.shared);
      while(rl.active_writer)
        rl.readerQ.wait(lk);
      ++rl.active_readers;
      lk.unlock();
    }

    void lockExclusive(KeyType key) {
      auto &rl = locks[key];
      std::unique_lock<std::mutex> lk(rl.shared);
      while(rl.active_readers != 0)
        rl.writerQ.wait(lk);
      rl.active_writer = true;
      lk.unlock();
    }

    void unlockShared(KeyType key) {
      auto &rl = locks[key];
      std::unique_lock<std::mutex> lk(rl.shared);
      --rl.active_readers;
      lk.unlock();
      rl.writerQ.notify_one();
    }

    void unlockExclusive(KeyType key) {
      auto &rl = locks[key];
      std::unique_lock<std::mutex> lk(rl.shared);
      rl.active_writer = false;
      rl.readerQ.notify_all();
      lk.unlock();
    }

  private:
    struct RowLock {
      std::mutex              shared;
      std::condition_variable readerQ;
      std::condition_variable writerQ;
      int                     active_readers;
      bool                    active_writer;
    };
    std::unordered_map<KeyType, RowLock> locks;
};



/*******************************************************************************
 * @brief Table is a class for storing a relation of tuples of the same type.
 *
 * Table implements a relational table for storing tuples of a given type
 * @c RecordType which are indexed by the key of type @c KeyType.
 * Table supports inserting, updating, deleting of tuples as well as scans
 * within a transactional context (not yet implemented).
 *
 * @tparam RecordType
 *         the data type of the tuples (typically a TuplePtr or Tuple)
 * @tparam KeyType
 *         the data type of the key column (default = int)
 ******************************************************************************/
template <typename RecordType, typename KeyType>
class MVCCTable : public BaseTable,
                  public std::enable_shared_from_this<MVCCTable<RecordType,
                                                                KeyType>> {
  using TupleType = typename RecordType::Base;
  using SCtxType = StateContext<MVCCTable<RecordType, KeyType>>;
  using WriteSetType = WriteSet<KeyType, TupleType>;

 public:
#ifdef USE_ROCKSDB_TABLE
  using Table = RDBTable<pfabric::Tuple<MVCCObject<TupleType>>, KeyType>;
#elif USE_NVM_TABLES
  using Table = PBPTreeTable<pfabric::Tuple<MVCCObject<TupleType>>, KeyType>;
#else
  using Table = CuckooTable<pfabric::Tuple<MVCCObject<TupleType>>, KeyType>;
#endif

  /** For external access to template parameters */
  using RType = RecordType;
  using KType = KeyType;

  /** Alias for an updater function which returns a modification of the
   * parameter tuple
   */
  using UpdaterFunc = typename Table::UpdaterFunc;

  /** Aliases for a function performing updates + deletes. Similar to
   * UpdaterFunc it allows to update the tuple, but also to delete it
   * (indictated by the setting the bool component of @c UpdateResult to false)
   */
  using UpdelFunc = typename Table::UpdelFunc;
  using InsertFunc = typename Table::InsertFunc;

  /** Alias for an iterator to scan the table */
  using TableIterator = typename Table::TableIterator;

  /** Alias for a predicate evaluated using a scan: see @TableIterator for
    * details
    */
  using Predicate = typename Table::Predicate;

  /**
   * Constructors for creating an empty table.
   */
  MVCCTable(const TableInfo& tInfo, SCtxType& sCtx) noexcept(noexcept(Table(tInfo)))
    : BaseTable{tInfo}, tbl{tInfo}, sCtx{sCtx} {}
  MVCCTable(const std::string& tableName, SCtxType& sCtx) noexcept(noexcept(Table(tableName)))
    : tbl{tableName}, sCtx{sCtx} {}

  /**
    * Destructor for table.
    */
  ~MVCCTable() {}

  /*==========================================================================*
   * Transactional Operations                                                 *
   *==========================================================================*/
  void registerState() {
    tblID = sCtx.registerState(this->shared_from_this());
  }

  TableID getID() const {
    return tblID;
  }

  void transactionBegin(const TransactionID& txnID) {
    sCtx.txCntW++;
    writeSet.txnID = txnID;
  }

  Errc transactionPreCommit(const TransactionID& txnID) {
    /* MVCC + 2PC */
    TableID otherID = (tblID == 0) ? 1 : 0;

    auto& thisState = sCtx.getWriteStatus(txnID, tblID);
    const auto& otherState = sCtx.getWriteStatus(txnID, otherID);;
    Errc s = Errc::SUCCESS;
    thisState = Status::Commit;

    if(otherState == Status::Commit) {
      s = this->transactionCommit(txnID);
      if (s != Errc::SUCCESS) return s;
      s = sCtx.regStates[otherID]->transactionCommit(txnID);
      sCtx.setLastCTS(0, txnID);
      sCtx.removeTx(txnID);
    }

    /* MVCC without 2PC *//*
    Errc s = this->transactionCommit(txnID);
    if (tblID == 1) sCtx.removeTx(txnID);
    */

    return s;
  }

  Errc transactionCommit(const TransactionID& txnID) {
    using MVCCTuple = MVCCObject<TupleType>;

    const auto numEntries = writeSet.set.size();

    /* Buffer new MVCC entries */
    /// Remove duplicates
    std::sort(writeSet.set.begin(), writeSet.set.end());
    auto lastIt = std::unique(writeSet.set.begin(), writeSet.set.end(),
      [](const typename WriteSetType::Pair &a, const typename WriteSetType::Pair &b) -> bool {
        return (a.first == b.first);
      }
    );
    writeSet.set.erase(lastIt, writeSet.set.end());

    /// Update/Insert all entries from writeSet in underlying table
    /// In-place variant ======================================================
		///*
    for (const auto &e : writeSet.set) {
      /// If entry exists
      const auto success = updateByKey(e.first, [this, &txnID, &e](std::tuple<MVCCTuple> &tp) {
        MVCCTuple &mvcc = std::get<0>(tp);
        const auto dPos = mvcc.getCurrent(txnID);
        auto iPos = getFreePos(mvcc.usedSlots.load());
        while (iPos > MVCCTuple::Versions - 1) {
          /// If all version slots are occupied, old unused versions must be removed; this is the
          /// only necessary possible waiting point
          mvcc.cleanUpVersions(sCtx.recalcOldestVisible(txnID));
          iPos = getFreePos(mvcc.usedSlots.load());
        }

        /// No need for synchronization; only possible problem is if readers access at the same time
        /// they could end up with no fitting version and would restart (but no consistency or
        /// failure-atomicity issue here)
        mvcc.newEntry(txnID, iPos, dPos, e.second);
      });
      /// Entry does not exist yet
      if (!success) {
        MVCCObject<TupleType> mvcc{};
        mvcc.headers[0].cts.store(txnID);
        mvcc.headers[0].dts.store(DTS_INF);
        mvcc.values[0] = std::move(e.second);
        mvcc.usedSlots.store(1);
        /// Doesn't need synchronization in our use case as inserts only happen during preparation
        tbl.insert(std::move(e.first), std::move(mvcc));
      }
    }//*/
    /// Out-of-place variant ===================================================
    /*struct KeyMVCCPair {
      KeyType key;
      MVCCObject<TupleType> mvcc;
    };
    std::vector<KeyMVCCPair> newEntries(numEntries);

    /// Buffer new MVCC entries
    int i = 0;
    for(const auto &e : writeSet.set) {
      newEntries[i].key = e.first;
      /// if entry exists
      SmartPtr<pfabric::Tuple<MVCCTuple>> tplPtr;
      if (tbl.getByKey(e.first, tplPtr)) {
        newEntries[i].mvcc = ns_types::get<0>(*tplPtr);
        auto &last = newEntries[i].mvcc;
        const auto dPos = last.getCurrent(txnID);
        auto iPos = getFreePos(last.usedSlots);
        while (iPos > MVCCTuple::Versions - 1) {
          /// If all version slots are occupied, old unused versions must be removed
          last.cleanUpVersions(sCtx.recalcOldestVisible(txnID));
          iPos = getFreePos(last.usedSlots);
        }
        last.headers[dPos].dts = txnID;
        last.headers[iPos].cts = txnID;
        last.headers[iPos].dts = DTS_INF;
        last.values[iPos] = e.second;
        last.usedSlots |= (1LL << iPos);
      } else {
      /// Entry does not exist yet
        newEntries[i].mvcc = MVCCTuple();
        auto &last = newEntries[i].mvcc;
        last.headers[0].cts.store(txnID);
        last.headers[0].dts.store(DTS_INF);
        last.values[0] = std::move(e.second);
        last.usedSlots.store(1);
      }
      ++i;
    }

    /// Lock Exclusively for overwriting
    /// Write new Entries
    /// Unlock all
		auto pop = pool_by_pptr(tbl.btree);
    transaction::run(pop, [&] {
      for(const auto &e : newEntries) {
        locks.lockExclusive(e.key);
				tbl.insert(std::move(e.key), std::move(e.mvcc));
        locks.unlockExclusive(e.key);
      }
		});*/

    /* Clean up */
    writeSet.clean();
    return Errc::SUCCESS;
  }

  void transactionAbort(const TransactionID& txnID) {
    writeSet.clean();
  }

  Errc readCommit(const TransactionID txnID, KeyType* keys, size_t until) {
    /// nothing to do here
    return Errc::SUCCESS;
  }

  void cleanUpReads(KeyType* keys, size_t until) {
    /// nothing to do here
  }

  /*==========================================================================*
   * Table Operations                                                         *
   *==========================================================================*/

  /**
   * @brief Insert or update a tuple.
   *
   * Insert or update the given tuple @rec with the given key into the table.
   * If the key already exists then the tuple in the table is updated, otherwise
   * the tuple is newly inserted.
   * After the insert/update all observers are notified.
   *
   * @param key the key value of the tuple
   * @param rec the actual tuple
   */
  Errc insert(const TransactionID& txnID, KeyType key, const RecordType& rec) {
    writeSet.insert(key, rec.data());

    return Errc::SUCCESS;
  }

  Errc insert(const TransactionID& txnID, KeyType key, RecordType&& rec) {
    writeSet.insert(key, std::move(rec.data()));
    return Errc::SUCCESS;
  }

  /**
   * @brief Delete a tuple.
   *
   * Delete the tuples associated with the given key from the table
   * and inform the observers.
   *
   * @param key the key for which the tuples are deleted from the table
   * @return the number of deleted tuples
   */
  unsigned long deleteByKey(const TransactionID& txnID, KeyType key) {
    // TODO:  Tx support
    //writeSet.set.erase(key);
    return 1;
  }

  /**
   * @brief Delete all tuples satisfying a predicate.
   *
   * Delete all tuples from the table which satisfy the given predicate.
   *
   * @param func a predicate function returning true if the given tuple should
   * be
   *             deleted
   * @return the number of deleted tuples
   */
  unsigned long deleteWhere(Predicate func) {
    // TODO: Tx support
    return tbl.deleteWhere(func);
  }

  /**
   * @brief Update or delete the tuple specified by the given key.
   *
   * Update or delete the tuple in the table associated with the given key.
   * The actual modification is done by the updater function specified as
   * parameter.
   *
   * @param key the key of the tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple + a bool value indicating whether the tuple shall be kept
   * (=true)
   *        or deleted (=false)
   * @return the number of modified tuples
   */
  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc, InsertFunc ifunc = nullptr) {
    // TODO: Tx support
    // return tbl.updateOrDeleteByKey(key, ufunc, ifunc);
    return 0;
  }

  /**
   * @brief Update the tuple specified by the given key.
   *
   * Update the tuple in the table associated with the given key.
   * The actual modification is done by the updater function specified as
   * parameter.
   *
   * @param key the key of the tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple
   * @return the number of modified tuples
   */
  unsigned long updateByKey(KeyType key, UpdaterFunc ufunc) {
    // TODO: Tx support
    return tbl.updateByKey(key, ufunc);
  }

  /**
   * @brief Update all tuples satisfying the given predicate.
    *
   * Update all tuples in the table which satisfy the given predicate.
   * The actual modification is done by the updater function specified as
   * parameter.
   *
   * @param pfunc a predicate func returning true for a tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple
   * @return the number of modified tuples
   */
  unsigned long updateWhere(Predicate pfunc, UpdaterFunc ufunc) {
    // TODO: Tx support
    return tbl.updateWhere(pfunc, ufunc);
  }

  /**
   * @brief Return the tuple associated with the given key.
   *
   * Return the tuple from the table that is associated with the given
   * key. If the key doesn't exist, an exception is thrown.
   *
   * @param key the key value
   * @return the tuple associated with the given key
   */
  Errc getByKey(TransactionID txnID, KeyType key, SmartPtr<RecordType> &outValue) {
    /* Read own version if available */
    if(writeSet.txnID == txnID) {
      const auto it = std::find_if(writeSet.set.begin(), writeSet.set.end(),
          [&](const std::pair<KeyType, RecordType>& e) {return e.first == key;});
      if(it != writeSet.set.end()) {
        outValue.reset(new RecordType(it->second));
        return Errc::SUCCESS;
      }
    }
    /* Get MVCC Object */
    /// in-place variant
    ///*
    std::tuple<MVCCObject<TupleType>> * tplPtr = nullptr;
    if (!tbl.getAsRef(key, &tplPtr)) {
      return Errc::NOT_FOUND;
    }
    auto& mvcc = ns_types::get<0>(*tplPtr);//*/
    /// out-of-place variant
    /*
    locks.lockShared(key);
    std::tuple<MVCCObject<TupleType>> * tplPtr = nullptr;
    if (!tbl.getAsRef(key, &tplPtr)) {
      locks.unlockShared(key);
      return Errc::NOT_FOUND;
    }
    const auto &mvcc = ns_types::get<0>(*tplPtr);
    locks.unlockShared(key);*/

    /* Get read CTS (version that was read first) for consistency */
    auto& readCTS = sCtx.getReadCTS(txnID, 0);
    if(readCTS == 0) {
      /* first read operation by this txnID --> save snapshot version */
      readCTS = sCtx.getLastCTS(0);
    }

    /* Read visible tuple,
     * uses readCTS instead of txnID to avoid reading of delayed writes */
    const auto pos = mvcc.getCurrent(readCTS);
    if (pos == -1) {
      return Errc::NOT_FOUND;
    }

    /* Set out and return value */
    outValue.reset(new RecordType(mvcc.values[pos]));
    return Errc::SUCCESS;
  }

  /**
   * @brief Return a pair of iterators for scanning the table with a
   *        selection predicate.
   *
   * Return a begin and end iterator that allows to scan the whole table
   * and visiting only tuples which satisfy the given predicate
   * as in the following example:
   * @code
   * auto handle = testTable->select([](const MyTuplePtr& tp) {
   *                 return get<0>(tp) % 2 == 0;
   *               });
   * for (auto i = handle.first; i != handle.second; i++)
   *    // do something with *i
   * @endcode
   *
   * @param func a function pointer to a predicate
   * @return a pair of iterators
   */
  TableIterator select(Predicate func) { return tbl.select(func); }

  /**
   * @brief Return a pair of iterators for scanning the whole table.
   *
   * Return a begin and end iterator that allows to scan the whole table
   * as in the following example:
   * @code
   * auto handle = testTable->select();
   * for (auto i = handle.first; i != handle.second; i++)
   *    // do something with *i
   * @endcode
   *
   * @return a pair of iterators
   */
  TableIterator select() { return tbl.select(); }

  /**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   */
  unsigned long size() const { return tbl.size(); }

  void drop() { tbl.drop(); }
  void truncate() { tbl.truncate(); }

 private:


  /*==========================================================================*
   * Members                                                                  *
   *==========================================================================*/
	//RWLocks<KeyType> locks;
  WriteSetType writeSet;
  Table tbl;
  TableID tblID;
  SCtxType& sCtx;

}; /* end class MVCCTable */


} /* end namespace pfabric */

#endif /* end ifndef MVCCTable_hpp_*/
