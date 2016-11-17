/*
 * Copyright (c) 2014-16 The PipeFabric team,
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

namespace pfabric {

class TableException : public std::exception {
  std::string msg;

public:
    TableException(const char *s = "") : msg(s) {}

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

  virtual ~BaseTable() {}
};

template <typename RecordType, typename KeyType = DefaultKeyType>
class Table : public BaseTable {
public:
  typedef std::unordered_map<KeyType, RecordType> TableMap;
  typedef std::pair<RecordType, bool> UpdateResult;

  typedef std::function<UpdateResult(const RecordType&)> UpdelFunc;

  typedef std::function<RecordType(const RecordType&)> UpdaterFunc;

  typedef boost::signals2::signal<void (const RecordType&, TableParams::ModificationMode)> ObserverCallback;

  typedef FilterIterator<typename TableMap::iterator> TableIterator;
  typedef typename TableIterator::Predicate Predicate;


  Table() {}

  ~Table() { std::cout << "deallocate table" << std::endl; }

  void insert(KeyType key, const RecordType& rec) throw (TableException) {
    {
      std::lock_guard<std::mutex> lock(mMtx);
      mDataTable.insert({key, rec});
    }
    notifyObservers(rec, TableParams::Insert, TableParams::Immediate);
  }

  unsigned long deleteByKey(KeyType key) {
    unsigned long res = 0;
    {
      std::lock_guard<std::mutex> lock(mMtx);
      if (!mImmediateObservers.empty()) {
        auto res = mDataTable.find(key);
        if (res != mDataTable.end())
          notifyObservers(res->second, TableParams::Delete, TableParams::Immediate);
      }
      res = mDataTable.erase(key);
    }
    return res;
  }

  unsigned long deleteWhere(Predicate func) {
    unsigned long num = 0;
    for(auto it = mDataTable.begin(); it != mDataTable.end(); ) {
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

  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc) {
    std::unique_lock<std::mutex> lock(mMtx);

    auto res = mDataTable.find(key);
    if (res != mDataTable.end()) {
      TableParams::ModificationMode mode = TableParams::Update;
      unsigned long num = 1;

      auto updateRes = ufunc(res->second);

      if (updateRes.second)
        mDataTable[key] = updateRes.first;
      else {
        num = mDataTable.erase(key);
        mode = TableParams::Delete;
      }
      lock.unlock();
      notifyObservers(updateRes.first, mode, TableParams::Immediate);
      return num;
    }
    else {
      lock.unlock();
    }
    return 0;
  }

  unsigned long updateByKey(KeyType key, UpdaterFunc ufunc) {
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

  unsigned long updateWhere(Predicate pfunc, UpdaterFunc ufunc) {
    unsigned long num = 0;
    for(auto it = mDataTable.begin(); it != mDataTable.end(); it++) {
      if (pfunc(it->second)) {
        auto rec = ufunc(it->second);
        mDataTable[it->first] = rec;
        notifyObservers(rec, TableParams::Update, TableParams::Immediate);
        num++;
      }
    }
    return num;
  }

  const RecordType& getByKey(KeyType key) throw (TableException) {
    auto res = mDataTable.find(key);
    if (res != mDataTable.end())
      return res->second;
    else
      throw TableException();
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
    return make_pair(makeFilterIterator(mDataTable.begin(), func),
                      makeFilterIterator(mDataTable.end(), func));
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
    return make_pair(makeFilterIterator(mDataTable.begin(), alwaysTrue),
                      makeFilterIterator(mDataTable.end(), alwaysTrue));
  }

  /**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   */
  unsigned long size() const { return mDataTable.size(); }

  void registerObserver(typename ObserverCallback::slot_type const& cb, TableParams::NotificationMode mode) {
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
  void notifyObservers(const RecordType& rec,
    TableParams::ModificationMode mode, TableParams::NotificationMode notify) {
    if (notify == TableParams::Immediate) {
      mImmediateObservers(rec, mode);
    }
    else {
      mDeferredObservers(rec, mode);
    }
  }

  mutable std::mutex mMtx;
  TableMap mDataTable;
  ObserverCallback mImmediateObservers, mDeferredObservers;
};

}

#endif
