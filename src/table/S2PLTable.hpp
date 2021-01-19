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

#ifndef S2PLTable_hpp_
#define S2PLTable_hpp_

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
#include "table/RDBTable.hpp"
#elif USE_NVM_TABLES
#include "table/PBPTreeTable.hpp"
#else
#include "table/CuckooTable.hpp"
#include "table/HashMapTable.hpp"
#endif

#include "core/serialize.hpp"

#include "table/BaseTable.hpp"
#include "table/StateContext.hpp"
#include "table/TableException.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {


template <typename KeyType>
class S2PLLocks {
  //cf. https://stackoverflow.com/a/28121513
  public:
    int lockShared(KeyType key) {
      auto &rl = locks[key];
      std::unique_lock<std::mutex> lk(rl.shared);
      if(rl.active_writer) {
        lk.unlock();
        return -1;
      }
      ++rl.active_readers;
      lk.unlock();
      return 0;
    }

    void lockExclusive(KeyType key) {
      auto &rl = locks[key];
      std::unique_lock<std::mutex> lk(rl.shared);
      rl.active_writer = true;
      while(rl.active_readers != 0)
        rl.writerQ.wait(lk);
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
      //rl.readerQ.notify_all();
      lk.unlock();
    }

  private:
    struct RowLock {
      std::mutex              shared{};
      //std::condition_variable readerQ;
      std::condition_variable writerQ{};
      int                     active_readers{0};
      bool                    active_writer{false};
    };
    std::unordered_map<KeyType, RowLock> locks;
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
template <typename RecordType, typename KeyType = DefaultKeyType>
class S2PLTable : public BaseTable,
                  public std::enable_shared_from_this<S2PLTable<RecordType,
                                                                KeyType>> {
  using SCtxType = StateContext<S2PLTable<RecordType, KeyType>>;

 public:
#ifdef USE_ROCKSDB_TABLE
  using Table = RDBTable<RecordType, KeyType>;
#elif USE_NVM_TABLES
  using Table = PBPTreeTable<RecordType, KeyType>;
#else
  using Table = CuckooTable<RecordType, KeyType>;
#endif

  /** For external access to template parameters */
  using RType = RecordType;
  using KType = KeyType;

  /** alias for a predicate evaluated using a scan */
  //using Predicate = std::function<bool(const RecordType&)>;

  /** alias for a updater function which returns a modification of the
   * parameter tuple */
  using UpdaterFunc = typename Table::UpdaterFunc;

  /** aliases for a function performing updates + deletes. Similar to
   * UpdaterFunc, it allows to update the tuple, but also to delete it
   * (indictated by the setting the bool component of @c UpdateResult to false)
   */
  using UpdelFunc = typename Table::UpdelFunc;
  using InsertFunc = typename Table::InsertFunc;

  /** alias for an iterator to scan the table */
  using TableIterator = typename Table::TableIterator;

  /** alias for a predicate evaluated using a scan: see @TableIterator for
   * details */
  using Predicate = typename Table::Predicate;

  explicit S2PLTable(const TableInfo& tInfo, SCtxType& sCtx) noexcept(noexcept(Table(tInfo)))
      : BaseTable(tInfo), tbl(tInfo), sCtx{sCtx} {}

  explicit S2PLTable(const std::string& tableName, SCtxType& sCtx) noexcept(noexcept(Table(tableName)))
      : tbl(tableName), sCtx{sCtx} {}

  /**
   * Constructor for creating an empty table.
   */
  explicit S2PLTable(const std::string& tableName) : tbl{tableName}, sCtx{} {}

  /**
    * Destructor for table.
    */
  ~S2PLTable() {}

  /*==========================================================================*
   * Transactional Operations                                                 *
   *==========================================================================*/
  void registerState() {
    tblID = sCtx.registerState(this->shared_from_this());
  }

  void transactionBegin(const TransactionID& txnID) {
    sCtx.txCntW++;
    /*
    auto pop = pmem::obj::pool_by_pptr(tbl.q);
    if (tx == nullptr)
      tx = new transaction::manual(pop);
    */
  }

  Errc transactionPreCommit(const TransactionID& txnID) {
    TableID otherID = (tblID == 0) ? 1 : 0;

    auto& thisState = sCtx.getWriteStatus(txnID, tblID);
    auto& otherState = sCtx.getWriteStatus(txnID, otherID);;
    auto s = Errc::SUCCESS;
    thisState = Status::Commit;

    if(otherState == Status::Commit) {
      s = this->transactionCommit(txnID);
      if (s != Errc::SUCCESS) return s;
      s = sCtx.regStates[otherID]->transactionCommit(txnID);
      //transaction::commit();
      //delete tx;
      sCtx.removeTx(txnID);
    }
    return s;
  }

  Errc transactionCommit(const TransactionID& txnID) {
    for (const auto &k : wKeysLocked) {
      locks.unlockExclusive(k);
    }
    wKeysLocked.clear();
    return Errc::SUCCESS;
  }

  void transactionAbort(const TransactionID& txnID) {
    // theoretically undo
    for (const auto &k : wKeysLocked)
      locks.unlockExclusive(k);
    wKeysLocked.clear();
  }

  Errc readCommit(const TransactionID txnID, KeyType* keys, size_t until) {
    //nothing to do here
    return Errc::SUCCESS;
  }

  void cleanUpReads(const KeyType* keys, const size_t until) {
    for(auto i = 0u; i < until; ++i)
      locks.unlockShared(keys[i]);
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
  void insert(const TransactionID& txnID, KeyType key, const RecordType& rec) {
    //lockKey - wait
    locks.lockExclusive(key);
    wKeysLocked.push_back(key);
    // insert
#if USE_NVM_TABLES
    auto pop = pmem::obj::pool_by_pptr(tbl.q);
    transaction::run(pop, [&] {
      tbl.insert(key, rec);
    });
#else
    tbl.insert(key, rec);
#endif
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
    locks.lockExclusive(key);
    wKeysLocked.push_back(key);
    return tbl.deleteByKey(key);
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
  Errc getByKey(const TransactionID txnID, const KeyType key, SmartPtr<RecordType> &outValue) {
    if (locks.lockShared(key)) {
      return Errc::ABORT;
    }

    if (!tbl.getByKey(key, outValue)) {
      locks.unlockShared(key);
      return Errc::NOT_FOUND;
    }
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
  S2PLLocks<KeyType> locks;
  std::vector<KeyType> wKeysLocked;
  Table tbl;
  TableID tblID;
  SCtxType& sCtx;
};
}

#endif
