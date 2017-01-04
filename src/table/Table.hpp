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

#ifndef Table_hpp_
#define Table_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>

#include <mutex>
#include <shared_mutex>

#include <boost/signals2.hpp>

#include "fmt/format.h"

#include "table/FilterIterator.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {

/**
 * @brief An exception for signaling errors in table processing.
 *
 * TableExecption is an exception class for signaling errors while
 * processing a table.
 */
class TableException : public std::exception {
  std::string msg; //< a message string

public:
    /**
     * Construct a new TableException instance.
     *
     * @param s the message string
     */
    TableException(const char *s = "") : msg(s) {}

    /**
     * Returns th message string describing the exception.
     *
     * @return the message string
     */
    virtual const char* what() const throw() {
      return fmt::format("TableException: {}", msg).c_str();
    }
};

/**
 * Constants representing different modes for working with a table.
 */
struct TableParams {
  /**
   * NotificationMode specifies when a stream tuple is produced
   * from the table.
   */
  enum NotificationMode {
    Immediate, //< directly for each updated tuple
    OnCommit   //< on transaction commit
  };

  /**
   * ModificationMode describes the kind of modification that
   * triggered the tuple.
   */
  enum ModificationMode {
    Insert,  //< tuple was inserted into the table
    Update,  //< tuple was updated
    Delete   //< tuple was deleted
  };
};

/**
 * @brief BaseTable is the abstract base class for all table objects.
 */
class BaseTable {
protected:
  BaseTable() {}

  /**
   * Constructor for creating an empty table with a given schema.
   */
  BaseTable(const TableInfo& tInfo) : mTableInfo(std::make_shared<TableInfo>(tInfo)) {}

public:
  virtual ~BaseTable() {}

  /**
   * Return a pointer to the TableInfo object describing the schema of the table.
   *
   * @return a pointer to the corresponding TableInfo object
   */
  TableInfoPtr tableInfo() { return mTableInfo; }

protected:
  TableInfoPtr mTableInfo;    //< explicit schema information (can be empty)
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
class Table : public BaseTable {
public:
  //< the actual implementation of the table
  typedef std::unordered_map<KeyType, RecordType> TableMap;

  //< typedef for a updater function which returns a modification of the parameter tuple
  typedef std::function<RecordType(const RecordType&)> UpdaterFunc;

  //< typedefs for a function performing updates + deletes. Similar to UpdaterFunc
  //< it allows to update the tuple, but also to delete it (indictated by the
  //< setting the bool component of @c UpdateResult to false)
  typedef std::pair<RecordType, bool> UpdateResult;
  typedef std::function<UpdateResult(const RecordType&)> UpdelFunc;

  //< typedef for a callback function which is invoked when the table was updated
  typedef boost::signals2::signal<void (const RecordType&, TableParams::ModificationMode)> ObserverCallback;

  //< typedef for an iterator to scan the table
  typedef FilterIterator<typename TableMap::iterator> TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for details
  typedef typename TableIterator::Predicate Predicate;

  /**
   * Constructor for creating an empty table.
   */
  Table() {}

  /**
   * Constructor for creating an empty table with a given schema.
   */
  Table(const TableInfo& tInfo) : BaseTable(tInfo) {}

  /**
   * Destructor for table.
   */
  ~Table() {}

  /**
   * @brief Insert a tuple.
   *
   * Insert the given tuple @rec with the given key into the table. After the insert
   * all observers are notified.
   *
   * @param key the key value of the tuple
   * @param rec the actual tuple
   */
  void insert(KeyType key, const RecordType& rec) throw (TableException) {
    {
      // make sure we have exclusive access
      std::lock_guard<std::mutex> lock(mMtx);
      mDataTable.insert({key, rec});
    }
    // after the lock is released we can inform our observers
    notifyObservers(rec, TableParams::Insert, TableParams::Immediate);
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
  unsigned long deleteByKey(KeyType key) {
    unsigned long nres = 0;
    {
      // make sure we have exclusive access
      std::lock_guard<std::mutex> lock(mMtx);
      auto res = mDataTable.find(key);
      if (res != mDataTable.end()) {
        // if the key exists: notify our observers
        if (!mImmediateObservers.empty())
          notifyObservers(res->second, TableParams::Delete, TableParams::Immediate);
        // and delete the tuples
        nres = mDataTable.erase(key);
      }
    }
    return nres;
  }

  /**
   * @brief Delete all tuples satisfying a predicate.
   *
   * Delete all tuples from the table which satisfy the given predicate.
   *
   * @param func a predicate function returning true if the given tuple should be
   *             deleted
   * @return the number of deleted tuples
   */
  unsigned long deleteWhere(Predicate func) {
    // make sure we have exclusive access
    std::lock_guard<std::mutex> lock(mMtx);

    unsigned long num = 0;
    // we perform a full scan here ...
    for(auto it = mDataTable.begin(); it != mDataTable.end(); ) {
      // and check the predicate
      if (func(it->second)) {
        notifyObservers(it->second, TableParams::Delete, TableParams::Immediate);
        it = mDataTable.erase(it);
        num++;
      }
      else
        ++it;
    }
    return num;
  }

  /**
   * @brief Update or delete the tuple specified by the given key.
   *
   * Update or delete the tuple in the table associated with the given key.
   * The actual modification is done by the updater function specified as parameter.
   *
   * @param key the key of the tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple + a bool value indicating whether the tuple shall be kept (=true)
   *        or deleted (=false)
   * @return the number of modified tuples
   */
  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc) {
    // make sure we have exclusive access
    // note that we don't use a guard here!
    std::unique_lock<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end()) {
      TableParams::ModificationMode mode = TableParams::Update;
      unsigned long num = 1;

      // perform the update
      auto updateRes = ufunc(res->second);

      // check whether we have to perform an update ...
      if (updateRes.second)
        mDataTable[key] = updateRes.first;
      else {
        // or a delete
        num = mDataTable.erase(key);
        mode = TableParams::Delete;
      }
      lock.unlock();
      // notify the observers
      notifyObservers(updateRes.first, mode, TableParams::Immediate);
      return num;
    }
    else {
      // don't forget to release the lock
      lock.unlock();
    }
    return 0;
  }

  /**
   * @brief Update the tuple specified by the given key.
   *
   * Update the tuple in the table associated with the given key.
   * The actual modification is done by the updater function specified as parameter.
   *
   * @param key the key of the tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple
   * @return the number of modified tuples
   */
  unsigned long updateByKey(KeyType key, UpdaterFunc ufunc) {
    // make sure we have exclusive access
    std::unique_lock<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end()) {

      auto rec = ufunc(res->second);
      mDataTable[key] = rec;
      lock.unlock();
      notifyObservers(rec, TableParams::Update, TableParams::Immediate);
      return 1;
    }
    else {
      lock.unlock();
    }
    return 0;
  }

  /**
   * @brief Update all tuples satisfying the given predicate.
    *
   * Update all tuples in the table which satisfy the given predicate.
   * The actual modification is done by the updater function specified as parameter.
   *
   * @param pfunc a predicate func returning true for a tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple
   * @return the number of modified tuples
   */
  unsigned long updateWhere(Predicate pfunc, UpdaterFunc ufunc) {
    // make sure we have exclusive access
    std::lock_guard<std::mutex> lock(mMtx);

    unsigned long num = 0;
    // we perform a full table scan
    for(auto it = mDataTable.begin(); it != mDataTable.end(); it++) {
      // and check the predicate
      if (pfunc(it->second)) {
        auto rec = ufunc(it->second);
        mDataTable[it->first] = rec;
        notifyObservers(rec, TableParams::Update, TableParams::Immediate);
        num++;
      }
    }
    return num;
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
  const RecordType& getByKey(KeyType key) throw (TableException) {
    // make sure we have exclusive access
    std::unique_lock<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end())
      // if we found the tuple we just return it
      return res->second;
    else
      // otherwise an exception is raised
      throw TableException("key not found");
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
  std::pair<TableIterator, TableIterator> select(Predicate func) {
    return make_pair(makeFilterIterator(mDataTable.begin(), mDataTable.end(), func),
                      makeFilterIterator(mDataTable.end(), mDataTable.end(), func));
  }

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
  std::pair<TableIterator, TableIterator> select() {
    auto alwaysTrue = [](const RecordType&) { return true; };
    return make_pair(makeFilterIterator(mDataTable.begin(), mDataTable.end(), alwaysTrue),
                      makeFilterIterator(mDataTable.end(), mDataTable.end(), alwaysTrue));
  }

  /**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   */
  unsigned long size() const { return mDataTable.size(); }

  /**
   * @brief Register an observer
   *
   * Registers an observer (a slot) which is notified in case of updates on the table.
   *
   * @param cb the observer (slot)
   * @param mode the nofication mode (immediate or defered)
   */
  void registerObserver(typename ObserverCallback::slot_type const& cb,
    TableParams::NotificationMode mode) {
      switch (mode) {
        case TableParams::Immediate:
          mImmediateObservers.connect(cb);
          break;
        case TableParams::OnCommit:
          mDeferredObservers.connect(cb);
          break;
      }
  }

private:
  /**
   * @brief Perform the actual notification
   *
   * Notify all registered observers about a update.
   *
   * @param rec the modified tuple
   * @param mode the modification mode (insert, update, delete)
   * @param notify the nofication mode (immediate or defered)
   */
  void notifyObservers(const RecordType& rec,
    TableParams::ModificationMode mode, TableParams::NotificationMode notify) {
    if (notify == TableParams::Immediate) {
      mImmediateObservers(rec, mode);
    }
    else {
      // TODO: implement defered notification
      mDeferredObservers(rec, mode);
    }
  }

  mutable std::mutex mMtx; //< a mutex for getting exclusive access to the table
  TableMap mDataTable;     //< the actual table structure (a hash map)
  ObserverCallback mImmediateObservers, mDeferredObservers;
};

}

#endif
