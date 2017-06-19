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

#ifndef NVMTable_hpp_
#define NVMTable_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>
#include <stdint.h>
#include <unistd.h>

#include <boost/signals2.hpp>
#include <boost/array.hpp>

#include "fmt/format.h"

#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include "table/TableInfo.hpp"
#include "nvm/persistent_table.hpp"

#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/pool.hpp"
#include "nvml/include/libpmemobj++/transaction.hpp"

namespace pfabric {

using nvml::obj::delete_persistent;
using nvml::obj::make_persistent;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::pool;
using nvml::obj::transaction;
using pfabric::nvm::persistent_table;

template <typename RecordType, typename KeyType>
class NVMIterator
{
public:
  typedef std::function<bool(const RecordType &)> Predicate;
  typedef TuplePtr<RecordType> RecordTypePtr;
  typedef persistent_table<RecordType, KeyType> pTable_type;

  explicit NVMIterator() {}
  explicit NVMIterator(persistent_ptr<pTable_type> _pTable, Predicate _pred) : pTable(_pTable), pred(_pred)
  {
    // make sure the initial iterator position refers to an entry satisfying
    // the predicate
    //while (i != end && !pred(i->second))
    //  ++i;
  }

  NVMIterator &operator++()
  {
    //i++;
    //while (i != end && !pred(i->second))
    //  ++i;
    return *this;
  }

  NVMIterator operator++(int)
  {
    auto tmp = *this;
    //++(*this);
    return tmp;
  }

  bool isValid() const
  {
    return false; //i != end;
  }

  RecordTypePtr operator*()
  {
    TuplePtr<RecordType> tptr;
    //tptr.reset(pfabric::detail::retrieveTupel<RecordType>(block, pos));
    return tptr;
  }

protected:
  // Current ddc_block
  //persistent_ptr<struct ddc_block> block;
  // Current position in ddc_block
  //int32_t pos;
  //TODO: Make persistent_table handle current position
  // Persistent Table
  persistent_ptr<pTable_type> pTable;
  // Selection predicate
  Predicate pred;
};

template <typename RecordType, typename KeyType>
inline NVMIterator<RecordType, KeyType> makeNVMIterator(
    persistent_ptr<nvm::persistent_table<RecordType, KeyType>> pTable,
    typename NVMIterator<RecordType, KeyType>::Predicate pred)
{
  return NVMIterator<RecordType, KeyType>(pTable, pred);
}

/**
 * @brief NVMTable is a class for storing a relation of tuples of the same type.
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
class NVMTable : public BaseTable
{
public:
  // TODO: - Index (volatile)
  typedef nvm::persistent_table<RecordType, KeyType> pTable_type;

  struct root
  {
    persistent_ptr<pTable_type> pTable;
  };

  //< typedef for a updater function which returns a modification of the parameter tuple
  typedef std::function<void(RecordType &)> UpdaterFunc;

  //< typedefs for a function performing updates + deletes. Similar to UpdaterFunc
  //< it allows to update the tuple, but also to delete it (indictated by the
  //< setting the bool component of @c UpdateResult to false)
  typedef std::function<bool(RecordType &)> UpdelFunc;

  //< typedef for a callback function which is invoked when the table was updated
  typedef boost::signals2::signal<void(const RecordType &, TableParams::ModificationMode)> ObserverCallback;

  //< typedef for an iterator to scan the table
  typedef NVMIterator<RecordType, KeyType> TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for details
  typedef typename TableIterator::Predicate Predicate;

  /**
   * Constructor for creating an empty table.
   */
  NVMTable(const std::string& tableName)
  {
		//TODO: Generate Schema
    //openOrCreateTable(tableName);
		throw std::exception("NVML table without schema is not yet implemented");
  }

  /**
   * Constructor for creating an empty table with a given schema.
   */
  NVMTable(const TableInfo & tInfo) : BaseTable(tInfo)
  {
    openOrCreateTable(tInfo);
  }

  /**
   * Destructor for table.
   */
  ~NVMTable()
  {
    pop.close();
  }

  /**
   * @brief Insert a tuple.
   *
   * Insert the given tuple @rec with the given key into the table. After the insert
   * all observers are notified.
   *
   * @param key the key value of the tuple
   * @param rec the actual tuple
   */
  void insert(KeyType key, const RecordType &rec) throw(TableException)
  {
    {
      //TODO:
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
  unsigned long deleteByKey(KeyType key)
  {
    unsigned long nres = 0;
    {
      //TODO:
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
  unsigned long deleteWhere(Predicate func)
  {
    unsigned long num = 0;
    //TODO:
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
  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc)
  {
    //TODO:
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
  unsigned long updateByKey(KeyType key, UpdaterFunc ufunc)
  {
    //TODO:
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
  unsigned long updateWhere(Predicate pfunc, UpdaterFunc ufunc)
  {
    unsigned long num = 0;
    //TODO:
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
  const TuplePtr<RecordType> getByKey(KeyType key) throw(TableException)
  {
    //TODO:
    TuplePtr<RecordType> tptr;
    return tptr;
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
  TableIterator select(Predicate func)
  {
    return makeNVMIterator<RecordType, KeyType>(q->pTable, func);
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
  TableIterator select()
  {
    auto alwaysTrue = [](const RecordType &) { return true; };
    return makeNVMIterator<RecordType, KeyType>(q->pTable, alwaysTrue);
  }

  /**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   */
  unsigned long size() const
  {
    //TODO:
    return 0;
  }

  /**
   * @brief Register an observer
   *
   * Registers an observer (a slot) which is notified in case of updates on the table.
   *
   * @param cb the observer (slot)
   * @param mode the nofication mode (immediate or defered)
   */
  void registerObserver(typename ObserverCallback::slot_type const &cb,
                        TableParams::NotificationMode mode)
  {
    switch (mode)
    {
    case TableParams::Immediate:
      mImmediateObservers.connect(cb);
      break;
    case TableParams::OnCommit:
      mDeferredObservers.connect(cb);
      break;
    }
  }

  void drop()
  {
    //TODO:
  }

private:
  void openOrCreateTable(const TableInfo& tableInfo) /*throw(TableException)*/
  {
    std::string path = tableInfo.tableName() + ".db";
    if (access(path.c_str(), F_OK) != 0)
    {
      pop = pool<root>::create(path, nvm::LAYOUT);//, (size_t)blockSize, 0666);
      //throw TableException("failed to create pool\n");
    }
    else
    {
      pop = pool<root>::open(path, nvm::LAYOUT);
      //throw TableException("failed to open pool\n");
    }
    q = pop.get_root();
    if (!q->pTable)
    {
      transaction::exec_tx(
          pop, [&] { q->pTable = make_persistent<pTable_type>(tableInfo); });
    }
  }

  /**
   * @brief Perform the actual notification
   *
   * Notify all registered observers about a update.
   *
   * @param rec the modified tuple
   * @param mode the modification mode (insert, update, delete)
   * @param notify the nofication mode (immediate or defered)
   */
  void notifyObservers(const RecordType &rec,
                       TableParams::ModificationMode mode, TableParams::NotificationMode notify)
  {
    if (notify == TableParams::Immediate)
    {
      mImmediateObservers(rec, mode);
    }
    else
    {
      // TODO: implement defered notification
      mDeferredObservers(rec, mode);
    }
  }

  pool<root> pop;
  persistent_ptr<root> q;
  ObserverCallback mImmediateObservers, mDeferredObservers;

}; /* class NVMTable */

} /* namespace pfabric */

#endif /* NVMTable_hpp_ */
