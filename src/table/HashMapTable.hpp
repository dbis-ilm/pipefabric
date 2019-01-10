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

#ifndef HashMapTable_hpp_
#define HashMapTable_hpp_

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

#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {

template <typename Iter>
class HashMapIterator {
public:
  typedef typename Iter::value_type::second_type RecordType;

  typedef std::function<bool(const RecordType&)> Predicate;

  explicit HashMapIterator() {}
  explicit HashMapIterator(Iter j, Iter e, Predicate p) : i(j), end(e), pred(p) {
    // make sure the initial iterator position refers to an entry satisfying
    // the predicate
    while (i != end && ! pred(i->second)) ++i;
  }

  HashMapIterator& operator++() {
    i++;
    while (i != end && ! pred(i->second)) ++i;
    return *this;
  }

  HashMapIterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }
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
inline HashMapIterator<Iter> makeHashMapIterator(Iter j, Iter e,
  typename HashMapIterator<Iter>::Predicate p) { return HashMapIterator<Iter>(j, e, p); }

/**
 * @brief HashMapTable is a class for storing a relation of tuples of the same type.
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
class HashMapTable : public BaseTable {
public:
  //< the actual implementation of the table
  typedef std::unordered_map<KeyType, RecordType> TableMap;

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
  typedef HashMapIterator<typename TableMap::iterator> TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for details
  typedef typename TableIterator::Predicate Predicate;

  /**
   * Constructor for creating an empty table.
   */
  HashMapTable(const std::string& = "") {}

  /**
   * Constructor for creating an empty table with a given schema.
   */
  HashMapTable(const TableInfo& tInfo) : BaseTable(tInfo) {}

  /**
   * Destructor for table.
   */
  ~HashMapTable() {}

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
    {
      // make sure we have exclusive access
      std::lock_guard<std::mutex> lock(mMtx);
      auto iter = mDataTable.find(key);
      if (iter != mDataTable.end()) 
        // we erase the key/tuple pair first, because of the missing copy assignment
        // operator in Tuple we cannot simply use the operator[]
        mDataTable.erase(key);
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
  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc, InsertFunc ifunc = nullptr) {
    // make sure we have exclusive access
    // note that we don't use a guard here!
    std::unique_lock<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end()) {
      TableParams::ModificationMode mode = TableParams::Update;
      unsigned long num = 1;

      // perform the update
      auto upd = ufunc(res->second);

      // check whether we have to perform an update ...
      if (!upd) {
        // or a delete
        num = mDataTable.erase(key);
        mode = TableParams::Delete;
      }
      lock.unlock();
      // notify the observers
      notifyObservers(res->second, mode, TableParams::Immediate);
      return num;
    }
    else {
      // key doesn't exist
      if (ifunc != nullptr) {
        insert(key, ifunc());
        // don't forget to release the lock
        lock.unlock();
        return 1;
      } else {
        // release lock always after finished table altering
        lock.unlock();      
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
    // make sure we have exclusive access
    std::unique_lock<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end()) {
      ufunc(res->second);

      lock.unlock();
      notifyObservers(res->second, TableParams::Update, TableParams::Immediate);
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
        ufunc(it->second);

        notifyObservers(it->second, TableParams::Update, TableParams::Immediate);
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
    // make sure we have exclusive access
    std::lock_guard<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end()) {
      // if we found the tuple we return a TuplePtr containing a copy of it
      SmartPtr<RecordType> tptr (new RecordType(res->second));
      return tptr;
    }
    else
      // otherwise an exception is raised
      throw TableException("key not found");
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
    return makeHashMapIterator(mDataTable.begin(), mDataTable.end(), func);
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
    return makeHashMapIterator(mDataTable.begin(), mDataTable.end(), alwaysTrue);
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

  mutable std::mutex mMtx; //< a mutex for getting exclusive access to the table
  TableMap mDataTable;     //< the actual table structure (a hash map)
  ObserverCallback mImmediateObservers, mDeferredObservers;
};

}

#endif
