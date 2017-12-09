/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef TxTable_hpp_
#define TxTable_hpp_

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

#include "rocksdb/db.h"

#include "core/serialize.hpp"

#include "table/BaseTable.hpp"
#include "table/RDBTable.hpp"
#include "table/TableException.hpp"
#include "table/TableInfo.hpp"
#include "table/LogBuffer.hpp"

namespace pfabric {

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
class TxTable : public BaseTable {
 public:
  //< typedef for a predicate evaluated using a scan
  // typedef std::function<bool(const RecordType&)> Predicate;

  //< typedef for a updater function which returns a modification of the
  // parameter tuple
  typedef typename RDBTable<RecordType, KeyType>::UpdaterFunc UpdaterFunc;

  //< typedefs for a function performing updates + deletes. Similar to
  // UpdaterFunc
  //< it allows to update the tuple, but also to delete it (indictated by the
  //< setting the bool component of @c UpdateResult to false)
  typedef typename RDBTable<RecordType, KeyType>::UpdelFunc UpdelFunc;

  typedef typename RDBTable<RecordType, KeyType>::InsertFunc InsertFunc;

  //< typedef for an iterator to scan the table
  typedef typename RDBTable<RecordType, KeyType>::TableIterator TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for
  // details
  typedef typename RDBTable<RecordType, KeyType>::Predicate Predicate;

  TxTable(const TableInfo& tInfo) throw(TableException)
      : BaseTable(tInfo), rdbTable(tInfo) {
  }

  /**
   * Constructor for creating an empty table.
   */
  TxTable(const std::string& tableName) throw(TableException)
  : rdbTable(tableName) {
  }

  /**
    * Destructor for table.
    */
  ~TxTable() {}

  void transactionCommit(const TransactionID& txID) {
    // TODO: use a more efficient way (lock per transaction)
    std::lock_guard<std::mutex> guard(tblMtx);

    for (auto iter = logBuffer.begin(txID); iter != logBuffer.end(txID); iter++) {
      switch (iter->logOp) {
        case LogOp::Insert:
          rdbTable.insert(iter->key, *(iter->recordPtr));
          break;
        case LogOp::Update:
          // TODO
          break;
        case LogOp::Delete:
          rdbTable.deleteByKey(iter->key);
          break;
      }
    }
    logBuffer.cleanup(txID);
  }

  void transactionAbort(const TransactionID& txID) {
    logBuffer.cleanup(txID);
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
  void insert(const TransactionID& txID, KeyType key, const RecordType& rec) throw(TableException) {
    // Tx support
    logBuffer.append(txID, LogOp::Insert, key, rec);
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
    logBuffer.append(txID, LogOp::Delete, key);
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
    return rdbTable.deleteWhere(func);
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
    // return rdbTable.updateOrDeleteByKey(key, ufunc, ifunc);
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
    return rdbTable.updateByKey(key, ufunc);
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
    return rdbTable.updateWhere(pfunc, ufunc);
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
  SmartPtr<RecordType> getByKey(KeyType key) throw(TableException) { return rdbTable.getByKey(key); }

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
  TableIterator select(Predicate func) { return rdbTable.select(func); }

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
  TableIterator select() { return rdbTable.select(); }

  /**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   */
  unsigned long size() const { return rdbTable.size(); }

  void drop() { rdbTable.drop(); }

 private:
  std::mutex tblMtx;
  RDBTable<RecordType, KeyType> rdbTable;
  LogBuffer<KeyType, RecordType> logBuffer;
};
}

#endif
