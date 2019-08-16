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

#ifndef CuckooTable_hpp_
#define CuckooTable_hpp_

#include <iostream>
#include <vector>
#include <functional>
#include <exception>
#include <iterator>

#include <mutex>
#include <shared_mutex>

#include <boost/signals2.hpp>

#include "libcuckoo/cuckoohash_map.hh"

#include "fmt/format.h"

#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {

template <typename Iter>
class CuckooIterator {
public:
  typedef typename Iter::value_type::second_type RecordType;

  typedef std::function<bool(const RecordType&)> Predicate;

  explicit CuckooIterator() {}
  explicit CuckooIterator(Iter j, Iter e, Predicate p) : i(j), end(e), pred(p) {
    // make sure the initial iterator position refers to an entry satisfying
    // the predicate
    while (i != end && ! pred(i->second)) ++i;
  }

  CuckooIterator& operator++() {
    i++;
    while (i != end && ! pred(i->second)) ++i;
    return *this;
  }

  CuckooIterator operator++(int) { const auto tmp{*this}; ++(*this); return tmp; }
  bool isValid() const { return i != end; }
  SmartPtr<RecordType> operator*() {
    return SmartPtr<RecordType> (new RecordType(i->second));
  }
  // typename Iter::value_type::second_type* operator->() { return &i->second; }

protected:
  Iter i, end;
  Predicate pred;
};

template <typename Iter>
inline CuckooIterator<Iter> makeCuckooIterator(Iter j, Iter e,
  typename CuckooIterator<Iter>::Predicate p) { return CuckooIterator<Iter>(j, e, p); }

/**
 * @brief CuckooTable is a class for storing a relation of tuples of the same type.
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
class CuckooTable : public BaseTable {
public:
  //< the actual implementation of the table
  typedef cuckoohash_map<KeyType, RecordType> TableMap;

  //< typedef for a updater function which returns a modification of the parameter tuple
  typedef std::function<void(RecordType&)> UpdaterFunc;

  //< typedefs for a function performing updates + deletes. Similar to UpdaterFunc
  //< it allows to update the tuple, but also to delete it (indictated by the
  //< setting the bool component of @c UpdateResult to false)
  typedef std::function<bool(RecordType&)> UpdelFunc;

  typedef std::function<RecordType()> InsertFunc;

  //< typedef for a callback function which is invoked when the table was updated
  typedef boost::signals2::signal<void (const RecordType&, TableParams::ModificationMode)> ObserverCallback;

  //< typedef for an iterator to scan the table
  typedef CuckooIterator<typename TableMap::locked_table::iterator> TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for details
  typedef typename TableIterator::Predicate Predicate;

  /**
   * Constructor for creating an empty table.
   */
  CuckooTable(const std::string& = "") {}

  /**
   * Constructor for creating an empty table with a given schema.
   */
  CuckooTable(const TableInfo& tInfo) : BaseTable(tInfo) {}

  /**
   * Destructor for table.
   */
  ~CuckooTable() {}

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
  void insert(KeyType key, const RecordType& rec) {
    mDataTable.insert(key, rec); //< calls upsert
    // we inform our observers
    notifyObservers(rec, TableParams::Insert, TableParams::Immediate);
  }


  void insert(KeyType key, RecordType&& rec) {
    mDataTable.insert(key, std::move(rec)); //< calls upsert
    // we inform our observers
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
    try {
      auto res = mDataTable.find(key);
      // if the key exists: notify our observers
      if (!mImmediateObservers.empty())
        notifyObservers(res, TableParams::Delete, TableParams::Immediate);
      // and delete the tuples
      mDataTable.erase_fn(key, [](RecordType r){return true;});
    } catch(std::out_of_range &e) {
      return 0;
    }
    return 1;
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
    unsigned long num = 0;
    // we perform a full scan here ...
    auto lt = mDataTable.lock_table();
    for(auto &it : lt) {
      // and check the predicate
      if (func(it.second)) {
        notifyObservers(it.second, TableParams::Delete, TableParams::Immediate);
        mDataTable.erase_fn(it.first, [](RecordType r){return true;});
        num++;
      }
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
  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc, InsertFunc ifunc = nullptr) {
    if (mDataTable.find_fn(key, [](RecordType r){})) {
      TableParams::ModificationMode mode = TableParams::Update;
      unsigned long num = 1;

      // perform the update
      auto res = mDataTable.find(key);
      auto upd = ufunc(res);

      // check whether we have to perform an update ...
      if (!upd) {
        // or a delete
        num = mDataTable.erase_fn(key, [](RecordType r){return true;});
        mode = TableParams::Delete;
      }
      // notify the observers
      notifyObservers(res, mode, TableParams::Immediate);
      return num;
    } else {
      // key doesn't exist
      if (ifunc != nullptr) {
        insert(key, ifunc());
        return 1;
      } 
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
    if (mDataTable.find_fn(key, [](RecordType r){})) {
      auto res = mDataTable.update_fn(key, ufunc);
      notifyObservers(res, TableParams::Update, TableParams::Immediate);
      return 1;
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
    unsigned long num = 0;
    
    // we perform a full table scan
    auto lt = mDataTable.lock_table();
    for(auto &it : lt) {
      // and check the predicate
      if (pfunc(it.second)) {
        ufunc(it.second);
        notifyObservers(it.second, TableParams::Update, TableParams::Immediate);
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
  const SmartPtr<RecordType> getByKey(KeyType key) {
    try {
      auto res = mDataTable.find(key);
      // if we found the tuple we return a TuplePtr containing a copy of it
      SmartPtr<RecordType> tptr (new RecordType(res));
      return tptr;
    } catch(std::out_of_range& e) {
      throw TableException("key not found");
    }
  }

  /**
   * @brief Return a pair of iterators for scanning the table with a
   *        selection predicate.
   *
   * Return an iterator that allows to scan the whole table
   * and visiting only tuples which satisfy the given predicate
   * as in the following example:
   * @code
   * auto iter = testTable->select([](const MyTuple& tp) {
   *                 return get<0>(tp) % 2 == 0;
   *               });
   * for (; iter.isValid(); i++)
   *    // do something with *i
   * @endcode
   *
   * @param func a function pointer to a predicate
   * @return a pair of iterators
   */
  TableIterator select(Predicate func) {
    return makeCuckooIterator(mDataTable.lock_table().begin(), mDataTable.lock_table().end(), func);
  }

  /**
   * @brief Return a pair of iterators for scanning the whole table.
   *
   * Return an iterator that allows to scan the whole table
   * as in the following example:
   * @code
   * auto iter = testTable->select();
   * for (; iter.isValid(); i++)
   *    // do something with *i
   * @endcode
   *
   * @return a pair of iterators
   */
  TableIterator select() {
    auto alwaysTrue = [](const RecordType&) { return true; };
    return makeCuckooIterator(mDataTable.lock_table().begin(), mDataTable.lock_table().end(), alwaysTrue);
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

  void drop() {
    mDataTable.clear();
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

  TableMap mDataTable;     //< the actual table structure (a hash map)
  ObserverCallback mImmediateObservers, mDeferredObservers;
};

}

#endif
