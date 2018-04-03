/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef MVCCTable_hpp_
#define MVCCTable_hpp_

#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>

#include "fmt/format.h"

#ifdef USE_ROCKSDB_TABLE
#include "rocksdb/db.h"
#include "table/RDBTable.hpp"
#else
#include "table/HashMapTable.hpp"
#endif

#include "core/serialize.hpp"

#include "table/BaseTable.hpp"
#include "table/TableException.hpp"
#include "table/TableInfo.hpp"
#include "table/LogBuffer.hpp"

namespace pfabric {

constexpr auto LEVEL_SNAPSHOT = 0;
constexpr auto LEVEL_SERIALIZABLE = 1;

constexpr auto NO_ERROR = 0;
constexpr auto ABORT = 1;
constexpr auto NOT_FOUND = 2;
constexpr auto INCONSISTENT = 3;

constexpr auto DTS_INF = std::numeric_limits<TransactionID>::max();

using TableID = unsigned int;

/* forward declaration*/
template <typename RecordType, typename KeyType = DefaultKeyType>
class MVCCTable;

enum class Status {Active, Commit, Abort};

template <typename RecordType, typename KeyType>
class StateContext {
  using ReadCTS = TransactionID;
  using TablePtr = std::shared_ptr<MVCCTable<RecordType, KeyType>>;
  using AccessedState = std::tuple<TableID, Status, ReadCTS>;
  using AccessedStates =  std::array<AccessedState, 2>;

 public:
  /** Atomic counter for assigning global transaction IDs */
  std::atomic<TransactionID> nextTxID{1};
  /** Registered States */
  TablePtr registeredStates[2];
  /** mapping from internal transaction ID to gloabl transaction ID */
  std::unordered_map<TransactionID, TransactionID> tToTX;
  /** for evaluation: counting necessary restarts of txs */
  std::atomic<unsigned int> restarts{0};

  bool running = true;

  AccessedState &getStateStatus(const TransactionID txnID,
                                const TableID tblID) {
    std::lock_guard<std::mutex> guard(mtx);
    return activeTxs.at(txnID).at(tblID);
  }

  const AccessedState &getStateStatus(const TransactionID txnID,
                                      const TableID tblID) const {
    std::lock_guard<std::mutex> guard(mtx);
    return activeTxs.at(txnID).at(tblID);
  }

  void removeTx(const TransactionID txnID) {
    std::lock_guard<std::mutex> guard(mtx);
    activeTxs.erase(txnID);
  }

  TransactionID newTx() {
    const auto txnID = nextTxID.fetch_add(1);
    std::lock_guard<std::mutex> guard(mtx);
    activeTxs.insert(std::make_pair(txnID, AccessedStates{
      AccessedState{0, Status::Active, 0},
      AccessedState{1, Status::Active, 0}}));
    return txnID;
  }

  TableID registerState(const TablePtr& tbl) {
    registeredStates[numStates++] = tbl;
    return numStates-1;
  }

  void reset() {
    std::lock_guard<std::mutex> guard(mtx);
    nextTxID.store(1);
    restarts.store(0);
    tToTX.clear();
//    activeTxs.clear();
    registeredStates[0] = nullptr;
    registeredStates[1] = nullptr;
    numStates = 0u;

  }

 private:
  /** mapping from transaction ID to list of accessed states */
  std::unordered_map<TransactionID, AccessedStates> activeTxs;
  unsigned int numStates = 0u;
  std::mutex mtx;
};

template <typename RecordType, std::size_t VERSIONS = 16>
class MVCCObject {
 public:
  std::uint64_t usedSlots;

  struct header {
    TransactionID cts;
    TransactionID dts;
    TransactionID rts;
  };

  std::array<header, VERSIONS> headers;
  std::array<RecordType, VERSIONS> values;

  int getCurrent(TransactionID txnID) const {
    for (auto i = 0u; i < VERSIONS; ++i) {
      if (usedSlots & (1 << i) &&
        headers[i].cts <= txnID && headers[i].dts > txnID)
        return i;
    }
    return -1;
  }
};

template <typename KeyType, typename RecordType>
struct WriteSet {
  TransactionID txnID;
  using Set = std::unordered_map<KeyType, RecordType>;
  Set set;

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

struct RowLock {
  //Type r/w
  std::mutex write;
  std::atomic<std::size_t> readers{0};
};

template <typename KeyType>
class SharedLocks {
 public:
  void lockShared(KeyType key) {
    auto &rowLock = locks[key];
    std::lock_guard<std::mutex> guard(rowLock.write); //< wait for possible writer
    rowLock.readers++;
//    locks[key].lock();
  }

  void lockExclusive(KeyType key) {
    auto &rowLock = locks[key];
    while(rowLock.readers > 0); //< wait for active readers
//    locks[key].lock();
  }

  void unlockShared(KeyType key) {
    locks[key].readers--;
//    locks[key].unlock();
  }

  void unlockExclusive(KeyType key) {
    locks[key].write.unlock();
//    locks[key].unlock();
  }

 private:
  std::unordered_map<KeyType, RowLock> locks;
//  std::unordered_map<KeyType, std::mutex> locks;
};



/**
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
 */
template <typename RecordType, typename KeyType>
class MVCCTable : public BaseTable,
                  public std::enable_shared_from_this<MVCCTable<RecordType,
                                                                KeyType>> {
  using TupleType = typename RecordType::Base;
  using SCtxType = StateContext<RecordType, KeyType>;
 public:

#ifdef USE_ROCKSDB_TABLE
  using Table = RDBTable<pfabric::Tuple<MVCCObject<TupleType>>, KeyType>;
#else
  using Table = HashMapTable<pfabric::Tuple<MVCCObject<TupleType>>, KeyType>;
#endif

  //< typedef for a predicate evaluated using a scan
  // typedef std::function<bool(const RecordType&)> Predicate;

  //< typedef for a updater function which returns a modification of the
  // parameter tuple
  using UpdaterFunc = typename Table::UpdaterFunc;

  // aliases for a function performing updates + deletes. Similar to
  // UpdaterFunc
  // it allows to update the tuple, but also to delete it (indictated by the
  // setting the bool component of @c UpdateResult to false)
  using UpdelFunc = typename Table::UpdelFunc;

  using InsertFunc = typename Table::InsertFunc;

  // alias for an iterator to scan the table
  using TableIterator = typename Table::TableIterator;

  // alias for a predicate evaluated using a scan: see @TableIterator for
  // details
  using Predicate = typename Table::Predicate;

  MVCCTable(const TableInfo &tInfo, SCtxType& sCtx) noexcept(noexcept(Table(tInfo)))
    : BaseTable{tInfo}, tbl{tInfo}, sCtx{sCtx} {}

  /**
   * Constructor for creating an empty table.
   */
  MVCCTable(const std::string &tableName, SCtxType& sCtx) noexcept(noexcept(Table(tableName)))
    : tbl{tableName}, sCtx{sCtx} {}

  /**
    * Destructor for table.
    */
  ~MVCCTable() {}

  void registerState() {
    tblID = sCtx.registerState(this->shared_from_this());
  }

  void transactionBegin(const TransactionID& txnID) {
    //sCtx.activeTxs[txnID]
    writeSet.txnID = txnID;
  }

  int transactionPreCommit(const TransactionID& txnID) {
    TableID otherID = (tblID == 0) ? 1 : 0;

    auto& thisState = sCtx.getStateStatus(txnID, tblID);
    auto& otherState = sCtx.getStateStatus(txnID, otherID);;
    int s = NO_ERROR;
    std::get<1>(thisState) =Status::Commit;

    if(std::get<1>(otherState) == Status::Commit) {
      s = this->transactionCommit(txnID);
      if (s != NO_ERROR) return s;
      s = sCtx.registeredStates[otherID]->transactionCommit(txnID);
    }
    return s;
  }

  int transactionCommit(const TransactionID& txnID) {
    struct KeyMVCCPair {
      KeyType key;
      MVCCObject<TupleType> mvcc;
    };
    //using KeyMVCCPair = std::pair<KeyType, MVCCObject<TupleType>>;
    const auto numEntries = writeSet.set.size();
    KeyMVCCPair *newEntries = new KeyMVCCPair[numEntries];

    /* Buffer new MVCC entries */
    int i = 0;
    for(const auto &e : writeSet.set) {
      locks.lockExclusive(e.first);
      /* if entry exists */
      try {
        newEntries[i] = KeyMVCCPair{e.first, get<0>(*tbl.getByKey(e.first))};
        auto &last = newEntries[i].mvcc;
        const auto iPos = getFreePos(last.usedSlots); //TODO: Check if higher than VERSIONS
        const auto dPos = last.getCurrent(txnID);
        if (isoLevel == LEVEL_SERIALIZABLE && last.headers[dPos].rts > txnID) {
          for (;i >= 0; i--)
            locks.unlockExclusive(newEntries[i].key); //< unlockAlL
          sCtx.restarts.fetch_add(1);
          return ABORT;
        }
        last.headers[dPos].dts = txnID;
        last.headers[iPos] = {txnID, DTS_INF, txnID};
        last.values[iPos] = e.second.data();
        last.usedSlots |= (1LL << iPos);
      }
      /* Entry does not exist yet */
      catch (TableException exc) {
        newEntries[i] = {e.first, MVCCObject<TupleType>()};
        auto &last = newEntries[i].mvcc;
        last.headers[0] = {txnID, DTS_INF, txnID};
        last.values[0] = e.second.data();
        last.usedSlots = 1;
      }
      ++i;
    }

    /* Write new Entries */
    for (auto e = 0u; e < numEntries; e++)
      tbl.insert(newEntries[e].key, newEntries[e].mvcc);

    lastCommitID = txnID;
    for (auto e = 0u; e < numEntries; e++)
      locks.unlockExclusive(newEntries[e].key); //< unlockAll
    writeSet.clean();
    delete[] newEntries;
    sCtx.removeTx(txnID);
    return NO_ERROR;
  }

  void transactionAbort(const TransactionID& txID) {
    writeSet.clean();
  }

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
  int insert(const TransactionID& txnID, KeyType key, const RecordType& rec) throw(TableException) {
    // Tx support
    if (isoLevel == LEVEL_SERIALIZABLE) {
      try {
        locks.lockShared(key);
        auto mvcc = get<0>(*tbl.getByKey(key));
        locks.unlockShared(key);
        auto pos = mvcc.getCurrent(txnID);
        if(mvcc.headers[pos].rts > txnID)
          return ABORT;
      } catch (TableException exc) {
        locks.unlockShared(key);
      }
    }
    //writeSet.txnID = txnID; //Assumes single writer
    writeSet.set.insert(std::make_pair(key,rec));
    return NO_ERROR;
  }

  int insert(const TransactionID& txnID, KeyType key, RecordType&& rec) {
    // Tx support
    if (isoLevel == LEVEL_SERIALIZABLE) {
      try {
        locks.lockShared(key);
        auto mvcc = get<0>(*tbl.getByKey(key));
        locks.unlockShared(key);
        auto pos = mvcc.getCurrent(txnID);
        if(mvcc.headers[pos].rts > txnID)
          return ABORT;
      } catch (TableException exc) {
        locks.unlockShared(key);
      }
    }
    //writeSet.txnID = txnID; //Assumes single writer
    writeSet.set.insert(std::make_pair(key,std::move(rec)));
    return NO_ERROR;
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
  unsigned long deleteByKey(const TransactionID& txID, KeyType key) {
    // Tx support
    writeSet.set.erase(key);
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
  int getByKey(TransactionID txnID, KeyType key, SmartPtr<RecordType> &outValue) {
    /* Read own version */
    if(writeSet.txnID == txnID && writeSet.set.find(key) != writeSet.set.end()) {
      outValue.reset(new RecordType(writeSet.set.at(key)));
      return NO_ERROR;
    }
    /* Get MVCC Object */
    if (isoLevel != LEVEL_SERIALIZABLE)
      locks.lockShared(key);
    else locks.lockExclusive(key);
    SmartPtr<pfabric::Tuple<MVCCObject<TupleType>>> tplPtr;
    try {
      tplPtr = tbl.getByKey(key);
    } catch (TableException exc) {
      if (isoLevel != LEVEL_SERIALIZABLE)
        locks.unlockShared(key);
      else locks.unlockExclusive(key);
      return NOT_FOUND;
    }
    /* check if read is in time */
    const auto& readCTS1 = std::get<2>(sCtx.getStateStatus(txnID, 0));
    const auto& readCTS2 = std::get<2>(sCtx.getStateStatus(txnID, 1));
    if ((readCTS1 != 0 && readCTS1 != lastCommitID) ||
        (readCTS2 != 0 && readCTS2 != lastCommitID)) {
      if (isoLevel != LEVEL_SERIALIZABLE)
        locks.unlockShared(key);
      else locks.unlockExclusive(key);
      return INCONSISTENT;
    }
    /* Update readCTS of this table*/
    std::get<2>(sCtx.getStateStatus(txnID, tblID)) = lastCommitID;
    auto& mvcc = get<0>(*tplPtr);
    if (isoLevel != LEVEL_SERIALIZABLE)
      locks.unlockShared(key);

    /* Read visible tuple */
    const auto pos = mvcc.getCurrent(txnID);
    if (pos == -1)
      return NOT_FOUND;
    if (isoLevel == LEVEL_SERIALIZABLE) {
      if (sCtx.running) { //TODO: dirty hack
        mvcc.headers[pos].rts = std::max(mvcc.headers[pos].rts, txnID);
        tbl.insert(key, *tplPtr);    // delete/insert
      }
      locks.unlockExclusive(key);
    }

    /* Set out and return value */
    outValue.reset(new RecordType(mvcc.values[pos]));
    return NO_ERROR;
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

 private:
  uint8_t getFreePos(const uint64_t v) {
    static constexpr uint8_t tab64[64] = {
      63, 0, 58, 1, 59, 47, 53, 2,
      60, 39, 48, 27, 54, 33, 42, 3,
      61, 51, 37, 40, 49, 18, 28, 20,
      55, 30, 34, 11, 43, 14, 22, 4,
      62, 57, 46, 52, 38, 26, 32, 41,
      50, 36, 17, 19, 29, 10, 13, 21,
      56, 45, 25, 31, 35, 16, 9, 12,
      44, 24, 15, 8, 23, 7, 6, 5};

    // Valid result is between 0 and 63
    // 64 means no free position
    if (v == UINT64_MAX) return 64;
    // Applying deBruijn hash function + lookup
    return tab64[((uint64_t) ((~v & -~v) * 0x07EDD5E59A4E28C2)) >> 58];
  }


  SharedLocks<KeyType> locks;
  TransactionID lastCommitID = 0;
  //uint8_t isoLevel = LEVEL_SERIALIZABLE;
  uint8_t isoLevel = LEVEL_SNAPSHOT;
  WriteSet<KeyType, RecordType> writeSet;
  Table tbl;
  TableID tblID;
  SCtxType& sCtx;
};
}

#endif
