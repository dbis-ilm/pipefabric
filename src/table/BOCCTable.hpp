/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOCCTable_hpp_
#define BOCCTable_hpp_

#include <atomic>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fmt/format.h"

#ifdef USE_ROCKSDB_TABLE
#include "rocksdb/db.h"
#include "table/RDBTable.hpp"
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
class BOCCTable : public BaseTable,
                  public std::enable_shared_from_this<BOCCTable<RecordType,
                                                                KeyType>> {
  using TupleType = typename RecordType::Base;
  using SCtxType = StateContext<BOCCTable<RecordType, KeyType>>;

 public:
#ifdef USE_ROCKSDB_TABLE
  using Table = RDBTable<RecordType, KeyType>;
#else
  using Table = CuckooTable<RecordType, KeyType>;
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

  /*******************************************************************************
   * @brief Write set for collecting uncommitted operations
   ******************************************************************************/
  struct ActiveWriteSet {
    TransactionID txnID;
    using Set = std::vector<std::pair<KeyType, RecordType>>;
    Set set;

    void insert(const KeyType& k, const RecordType& r) {
      set.emplace_back(k, r);
    }

    void insert(KeyType&& k, RecordType&& r) {
      set.emplace_back(std::move(k), std::move(r));
    }

    const RecordType& operator[](const KeyType& k) const {
      return set.at(k);
    }

    RecordType& operator[](const KeyType& k) {
      return set[k];
    }

    void clean() {
      txnID = 0;
      set.clear();
    }
  };

  /*******************************************************************************
   * @brief Write set for storing a set of keys and the period of activity
   ******************************************************************************/
  struct WriteSet {
    std::unordered_set<KeyType> keys; //< Write set
    TransactionID valTS;              //< Validation timestamp
    TransactionID endTS;              //< Completion timestamp

    WriteSet(const TransactionID _v, const TransactionID _e, const size_t _reserve = 0)
      noexcept : valTS{_v}, endTS{_e} { keys.reserve(_reserve); }
    void insert(const KeyType& k) { keys.emplace(k); }
    void insert(KeyType&& k) { keys.emplace(std::move(k)); }
  };

  /*******************************************************************************
   * @brief A read-write (shared) lock for handling the writeSet deque
   ******************************************************************************/
	class RWLock {
		//cf. https://stackoverflow.com/a/28121513
		public:
			void lockShared() {
				std::unique_lock<std::mutex> lk(shared);
				while(active_writer)
					readerQ.wait(lk);
				++active_readers;
				lk.unlock();
			}

			void lockExclusive() {
				std::unique_lock<std::mutex> lk(shared);
				while(active_readers)
					writerQ.wait(lk);
				active_writer = true;
				lk.unlock();
			}

			void unlockShared() {
				std::unique_lock<std::mutex> lk(shared);
				--active_readers;
				lk.unlock();
				writerQ.notify_one();
			}

			void unlockExclusive() {
				std::unique_lock<std::mutex> lk(shared);
				active_writer = false;
				readerQ.notify_all();
				lk.unlock();
			}

			std::mutex              shared{};
			std::condition_variable readerQ{};
			std::condition_variable writerQ{};
			int                     active_readers{0};
			bool                    active_writer{0};
	};

  /**
   * Constructors for creating an empty table.
   */
  BOCCTable(const TableInfo& tInfo, SCtxType& sCtx) noexcept(noexcept(Table(tInfo)))
    : BaseTable{tInfo}, tbl{tInfo}, sCtx{sCtx} {}
  BOCCTable(const std::string& tableName, SCtxType& sCtx) noexcept(noexcept(Table(tableName)))
    : tbl{tableName}, sCtx{sCtx} {}

  /**
    * Destructor for table.
    */
  ~BOCCTable() {}

  /*==========================================================================*
   * Transactional Operations                                                 *
   *==========================================================================*/
  void registerState() {
    tblID = sCtx.registerState(this->shared_from_this());
  }

  void transactionBegin(const TransactionID& txnID) {
    sCtx.txCntW++;
    writeSet.txnID = txnID;
  }

  Errc transactionPreCommit(const TransactionID& txnID) {
    TableID otherID = (tblID == 0) ? 1 : 0;

    auto& thisState = sCtx.getWriteStatus(txnID, tblID);
    const auto& otherState = sCtx.getWriteStatus(txnID, otherID);;
    Errc s = Errc::SUCCESS;
    thisState = Status::Commit;

    if(otherState == Status::Commit) {
      s = this->transactionCommit(txnID);
      if (s != Errc::SUCCESS) return s;
      s = sCtx.regStates[otherID]->transactionCommit(txnID);
      sCtx.removeTx(txnID);
    }
    return s;
  }

  Errc transactionCommit(const TransactionID& txnID) { 
    /* Apply changes and save writeSet */
    const auto valTS = sCtx.getNewTS();
    dQLock.lockExclusive();
    committedWSs.emplace_front(valTS, DTS_INF, writeSet.set.size());
    dQLock.unlockExclusive();
    auto& ws = committedWSs.front();
    for (const auto& e : writeSet.set) {
      tbl.insert(e.first, std::move(e.second));
      ws.insert(std::move(e.first));
    }
    ws.endTS = sCtx.getNewTS();
    writeSet.clean();

    /* Cleanup old write sets [remove all where EndTS < oldest active tx] */
//    if (committedWSs.size() > 100) {
      auto oldestTx = sCtx.getOldestActiveTx();
      if (oldestTx == txnID) oldestTx = ws.endTS;
      for(auto it = committedWSs.cbegin(); it != committedWSs.cend(); ++it) {
        if(it->endTS <= oldestTx) {
          dQLock.lockExclusive();
          committedWSs.erase(it, committedWSs.cend());
          dQLock.unlockExclusive();
          return Errc::SUCCESS;
        }
      }
//    }

    return Errc::SUCCESS;
  }

  void transactionAbort(const TransactionID& txnID) {
    writeSet.clean();
  }

  void cleanUpReads(KeyType* keys, size_t until) {
    //nothing to do here
  }

  /* BackwardValidation */
  Errc readCommit(const TransactionID txnID, KeyType* keys, size_t until) {
    /* Just for easier evaluation (--> always three timestamps per TX): */
    sCtx.getNewTS();
    const auto valTS = sCtx.getNewTS();
		dQLock.lockShared();
    for (const auto& ws: committedWSs) {
      if(txnID < ws.endTS && valTS > ws.valTS) { //< necessary condition
        for (auto i = 0u; i < until; ++i) {
          if(!ws.keys.empty() && ws.keys.find(keys[i]) != ws.keys.end()) { //< sufficient condition
            dQLock.unlockShared();
            return Errc::ABORT;
          }
        }
      }
    }
		dQLock.unlockShared();

    return Errc::SUCCESS;
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
  Errc insert(const TransactionID txnID, const KeyType key, const RecordType& rec) {
    writeSet.insert(key, rec);
    return Errc::SUCCESS;
  }

  Errc insert(const TransactionID txnID, const KeyType key, RecordType&& rec) {
    writeSet.insert(key, std::move(rec));
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
    
    SmartPtr<RecordType> tplPtr;
    try {
      tplPtr = tbl.getByKey(key);
    } catch (TableException& exc) {
      return Errc::NOT_FOUND;
    }
    outValue = tplPtr;
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

  void drop() { committedWSs.clear(); tbl.drop(); }
  void truncate() { committedWSs.clear(); tbl.truncate(); }

 private:


  /*==========================================================================*
   * Members                                                                  *
   *==========================================================================*/
  ActiveWriteSet writeSet;
  std::deque<WriteSet> committedWSs{};
 	RWLock dQLock;
  Table tbl;
  TableID tblID;
  SCtxType& sCtx;

}; /* end class BOCCTable */

} /* end namespace pfabric */

#endif /* end ifndef BOCCTable_hpp_*/
