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

#ifdef USE_ROCKSDB_TABLE
  typedef RDBTable<RecordType, KeyType> Table;
#else
  typedef HashMapTable<RecordType, KeyType> Table;
#endif

  using RType = RecordType;
  using KType = KeyType;

  //< typedef for a predicate evaluated using a scan
  // typedef std::function<bool(const RecordType&)> Predicate;

  //< typedef for a updater function which returns a modification of the
  // parameter tuple
  typedef typename Table::UpdaterFunc UpdaterFunc;

  //< typedefs for a function performing updates + deletes. Similar to
  // UpdaterFunc
  //< it allows to update the tuple, but also to delete it (indictated by the
  //< setting the bool component of @c UpdateResult to false)
  typedef typename Table::UpdelFunc UpdelFunc;

  typedef typename Table::InsertFunc InsertFunc;

  //< typedef for an iterator to scan the table
  typedef typename Table::TableIterator TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for
  // details
  typedef typename Table::Predicate Predicate;

  TxTable(const TableInfo& tInfo) : BaseTable(tInfo), tbl(tInfo) {}

  /**
   * Constructor for creating an empty table.
   */
  TxTable(const std::string& tableName) : tbl(tableName) {}

  /**
    * Destructor for table.
    */
  ~TxTable() {}

  void transactionBegin(const TransactionID& txID) {}

  void transactionPreCommit(const TransactionID& txID) {
    transactionCommit(txID);
  }

  void transactionCommit(const TransactionID& txID) {
    // TODO: use a more efficient way (lock per transaction)
    std::lock_guard<std::mutex> guard(tblMtx);

    for (auto iter = logBuffer.begin(txID); iter != logBuffer.end(txID); iter++) {
      switch (iter->logOp) {
        case LogOp::Insert:
          tbl.insert(iter->key, *(iter->recordPtr));
          break;
        case LogOp::Update:
          // TODO
          break;
        case LogOp::Delete:
          tbl.deleteByKey(iter->key);
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
  void insert(const TransactionID& txID, KeyType key, const RecordType& rec) {
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
  SmartPtr<RecordType> getByKey(KeyType key) { return tbl.getByKey(key); }

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
  std::mutex tblMtx;
  Table tbl;
  LogBuffer<KeyType, RecordType> logBuffer;
};
}

#endif
