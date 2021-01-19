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

#ifndef BDCCPTable_hpp_
#define BDCCPTable_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>
#include <stdint.h>
#include <unistd.h>
#include <cstdio>
#include <type_traits>

#include <boost/signals2.hpp>

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <libpmempool.h>

#include "fmt/format.h"
#include "ptable/PTable.hpp"

#include "pfabric_config.h"
#include "core/Tuple.hpp"
#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::transaction;
using dbis::ptable::PTable;
using dbis::ptable::PTuple;

template<typename KeyType, typename RecordType>
class BDCCPIterator {
 public:
  static_assert(is_tuple<RecordType>::value, "Value type must be a pfabric::Tuple");
  using TupleType = typename RecordType::Base;
//  using Predicate = std::function<bool(const PTuple<TupleType, KeyType> &)>;
  using Predicate = std::function<bool(const RecordType &)>;
  using PTableType = PTable<KeyType, TupleType>;

  explicit BDCCPIterator() {
  }

  explicit BDCCPIterator(typename PTableType::iterator &&_iter, typename PTableType::iterator &&_end, Predicate _pred) :
    iter(std::move(_iter)), end(std::move(_end)), pred(_pred) {

    while (isValid() && !pred(*(*iter).createTuple()))
      iter++;
  }

  BDCCPIterator &operator++() {
    iter++;
    while (isValid() && !pred(*(*iter).createTuple()))
      iter++;
    return *this;
  }

  BDCCPIterator operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool isValid() const {
    return iter != end;
  }

  SmartPtr<RecordType> operator*() {
    SmartPtr<RecordType> tptr(new RecordType(*(*iter).createTuple()));
    return tptr; //TODO: Is this to expensive?
  }

 protected:
  // PTable Iterator
  typename PTableType::iterator iter, end;
  // Selection predicate
  Predicate pred;

};

template<typename KeyType, typename RecordType>
inline BDCCPIterator<KeyType, RecordType> makeBDCCPIterator(
  typename PTable<KeyType, typename RecordType::Base>::iterator &&iter,
  typename PTable<KeyType, typename RecordType::Base>::iterator &&end,
  typename BDCCPIterator<KeyType, RecordType>::Predicate pred) {
  return BDCCPIterator<KeyType, RecordType>(std::move(iter), std::move(end), pred);
}

/**************************************************************************//**
 * \brief BDCCPTable is a class for storing a relation of tuples of the same type.
 *
 * Table implements a relational table for storing tuples of a given type
 * \c RecordType which are indexed by the key of type \c KeyType.
 * Table supports inserting, updating, deleting of tuples as well as scans
 * within a transactional context (not yet implemented).
 *
 * \tparam RecordType
 *         the data type of the tuples (typically a TuplePtr or Tuple)
 * \tparam KeyType
 *         the data type of the key column (default = int)
 *****************************************************************************/
template<typename RecordType, typename KeyType = DefaultKeyType>
class BDCCPTable : public BaseTable {
 public:
  static_assert(is_tuple<RecordType>::value, "Value type must be a pfabric::Tuple");
  using TupleType = typename RecordType::Base;
  using PTableType = PTable<KeyType, TupleType>;

  struct root {
    persistent_ptr<PTableType> pTable;
  };

  /** typedef for a updater function which returns a modification of the parameter tuple */
  using UpdaterFunc = std::function<void(RecordType &)>;

  /** typedefs for a function performing updates + deletes. Similar to UpdaterFunc
   *  it allows to update the tuple, but also to delete it (indictated by the
   *  setting the bool component of \c UpdateResult to false)
   **/
  using UpdelFunc = std::function<bool(RecordType &)>;

  using InsertFunc = std::function<RecordType()>;

  /** typedef for a callback function which is invoked when the table was updated */
  using ObserverCallback = boost::signals2::signal<void(const RecordType &, TableParams::ModificationMode)>;

  /** typedef for an iterator to scan the table */
  using TableIterator = BDCCPIterator<KeyType, RecordType>;

  /** typedef for a predicate evaluated using a scan: see \TableIterator for details */
  using Predicate = typename TableIterator::Predicate;

  /************************************************************************//**
   * \brief Constructor for creating an empty table with only a given name.
   *****************************************************************************/
  BDCCPTable(const std::string &tableName) : BaseTable(constructSchema<RecordType>(tableName)) {
    openOrCreateTable(constructSchema<RecordType>(tableName));
  }

  /************************************************************************//**
   * \brief Constructor for creating an empty table with a given schema.
   *****************************************************************************/
  BDCCPTable(const TableInfo &tInfo) :
    BaseTable(tInfo) {
    openOrCreateTable(tInfo);
  }

  /************************************************************************//**
   * \brief Destructor for table.
   *****************************************************************************/
  ~BDCCPTable() {
    // pop.close();
  }

  /************************************************************************//**
   * \brief Insert a tuple.
   *
   * Insert the given tuple \rec with the given key into the table. After the insert
   * all observers are notified.
   *
   * \param key the key value of the tuple
   * \param rec the actual tuple
   *****************************************************************************/
  void insert(KeyType key, const RecordType &rec) noexcept(false) {
    pTable->insert(key, rec.data());
    notifyObservers(rec, TableParams::Insert, TableParams::Immediate);
  }

  /************************************************************************//**
   * \brief Delete a tuple.
   *
   * Delete the tuples associated with the given key from the table
   * and inform the observers.
   *
   * \param key the key for which the tuples are deleted from the table
   * \return the number of deleted tuples
   *****************************************************************************/
  unsigned long deleteByKey(KeyType key) {
    return pTable->deleteByKey(key);
  }

  /************************************************************************//**
   * \brief Delete all tuples satisfying a predicate.
   *
   * Delete all tuples from the table which satisfy the given predicate.
   *
   * \param func a predicate function returning true if the given tuple should be
   *             deleted
   * \return the number of deleted tuples
   *****************************************************************************/
  unsigned long deleteWhere(Predicate func) {
    unsigned long num = 0;
    //TODO:
    return num;
  }

  /************************************************************************//**
   * \brief Update or delete the tuple specified by the given key.
   *
   * Update or delete the tuple in the table associated with the given key.
   * The actual modification is done by the updater function specified as parameter.
   *
   * \param key the key of the tuple to be modified
   * \param func a function performing the modification by returning a modified
   *        tuple + a bool value indicating whether the tuple shall be kept (=true)
   *        or deleted (=false)
   * \return the number of modified tuples
   *****************************************************************************/
  unsigned long updateOrDeleteByKey(KeyType key, UpdelFunc ufunc) {
    //TODO:
    return 0;
  }

  /************************************************************************//**
   * \brief Update the tuple specified by the given key.
   *
   * Update the tuple in the table associated with the given key.
   * The actual modification is done by the updater function specified as parameter.
   *
   * \param key the key of the tuple to be modified
   * \param func a function performing the modification by returning a modified
   *        tuple
   * \return the number of modified tuples
   *****************************************************************************/
  unsigned long updateByKey(KeyType key, UpdaterFunc ufunc) {
    //TODO:
    return 0;
  }

  /************************************************************************//**
   * \brief Update all tuples satisfying the given predicate.
   *
   * Update all tuples in the table which satisfy the given predicate.
   * The actual modification is done by the updater function specified as parameter.
   *
   * \param pfunc a predicate func returning true for a tuple to be modified
   * \param func a function performing the modification by returning a modified
   *        tuple
   * \return the number of modified tuples
   ***************************************************************************/
  unsigned long updateWhere(Predicate pfunc, UpdaterFunc ufunc) {
    unsigned long num = 0;
    //TODO:
    return num;
  }

  /************************************************************************//**
   * \brief Return the tuple associated with the given key.
   *
   * Return the tuple from the table that is associated with the given
   * key. If the key doesn't exist, an exception is thrown.
   *
   * \param key the key value
   * \return the tuple associated with the given key
   *****************************************************************************/
  const SmartPtr<RecordType> getByKey(KeyType key) noexcept(false) {
    //TODO: ugly, can we do better?
    try {
      SmartPtr<RecordType> tptr(new RecordType(*pTable->getByKey(key).createTuple()));
      return tptr;
    } catch (dbis::ptable::PTableException &pex) {
      throw TableException(pex.what());
    }
  }

  /************************************************************************//**
   * \brief Return a pair of iterators for scanning the table with a
   *        selection predicate.
   *
   * Return an iterator that allows to scan the whole table
   * and visiting only tuples which satisfy the given predicate
   * as in the following example:
   * \code
   * auto iter = testTable->select([](const MyTuple& tp) {
   *                 return get<0>(tp) % 2 == 0;
   *               });
   * for (; iter.isValid(); i++)
   *    // do something with *i
   * \endcode
   *
   * \param func a function pointer to a predicate
   * \return a pair of iterators
   *****************************************************************************/
  TableIterator select(Predicate func) {
    return makeBDCCPIterator<KeyType, RecordType>(std::move(pTable->begin()), std::move(pTable->end()), func);
  }

  /************************************************************************//**
   * \brief Return a pair of iterators for scanning the whole table.
   *
   * Return an iterator that allows to scan the whole table
   * as in the following example:
   * \code
   * auto iter = testTable->select();
   * for (; iter.isValid(); i++)
   *    // do something with *i
   * \endcode
   *
   * \return a pair of iterators
   *****************************************************************************/
  TableIterator select() {
    auto alwaysTrue = [](const RecordType &) { return true; };
    return makeBDCCPIterator<KeyType, RecordType>(std::move(pTable->begin()), std::move(pTable->end()), alwaysTrue);
  }

  /************************************************************************//**
   * \brief Return the number of tuples stored in the table.
   *
   * \return the number of tuples
   *****************************************************************************/
  unsigned long size() const {
    return pTable->count();
  }

  /************************************************************************//**
   * \brief Register an observer
   *
   * Registers an observer (a slot) which is notified in case of updates on the table.
   *
   * \param cb the observer (slot)
   * \param mode the nofication mode (immediate or defered)
   *****************************************************************************/
  void registerObserver(typename ObserverCallback::slot_type const &cb,
                        TableParams::NotificationMode mode) {
    switch (mode) {
      case TableParams::Immediate:mImmediateObservers.connect(cb);
        break;
      case TableParams::OnCommit:mDeferredObservers.connect(cb);
        break;
    }
  }

  void drop() {
    auto pop = pool_by_pptr(q);
    transaction::run(pop, [&] {
      delete_persistent<PTableType>(pTable);
      pTable = nullptr;
      delete_persistent<root>(q);
      q = nullptr;
    });
    pop.close();
    //pmempool_rm((pfabric::gPmemPath + BaseTable::mTableInfo->tableName() + ".db").c_str(), 1);
    std::remove((BaseTable::mTableInfo->tableName()+".db").c_str());
  }

  void truncate() {
    auto pop = pool_by_pptr(q);
    transaction::run(pop, [&] {
      delete_persistent<PTableType>(q->pTable);
      q->pTable = make_persistent<PTableType>();
      pTable = q->pTable;
    });
  }

  void print() {
    pTable->print(false);
  }

 private:
  void openOrCreateTable(const TableInfo &tableInfo) noexcept(false) {
    const std::string path = pfabric::gPmemPath + tableInfo.tableName() + ".db";
    pool<root> pop;

    if (access(path.c_str(), F_OK) != 0) {
      //TODO: How do we estimate the required pool size
      pop = pool<root>::create(path, dbis::ptable::LAYOUT, 64 * 1024 * 1024);
      transaction::run(pop, [&] {
          dbis::ptable::StringVector sVector;
        for (const auto &c : tableInfo) {
          sVector.emplace_back(c.getName());
        }
        dbis::ptable::VTableInfo<KeyType, TupleType> vTableInfo(tableInfo.tableName(), std::move(sVector));
        pop.root()->pTable = make_persistent<PTableType>(std::move(vTableInfo));
      });
    } else {
      pop = pool<root>::open(path, dbis::ptable::LAYOUT);
    }
    q = std::move(pop.root());
    pTable = q->pTable;
  }

  /************************************************************************//**
   * \brief Perform the actual notification
   *
   * Notify all registered observers about a update.
   *
   * \param rec the modified tuple
   * \param mode the modification mode (insert, update, delete)
   * \param notify the nofication mode (immediate or defered)
   *****************************************************************************/
  void notifyObservers(const RecordType &rec, TableParams::ModificationMode mode,
                       TableParams::NotificationMode notify) {
    if (notify == TableParams::Immediate) {
      mImmediateObservers(rec, mode);
    } else {
      // TODO: implement defered notification
      mDeferredObservers(rec, mode);
    }
  }

  persistent_ptr<struct root> q;
  persistent_ptr<PTableType> pTable;
  ObserverCallback mImmediateObservers, mDeferredObservers;

}; /* class BDCCPTable */

} /* namespace pfabric */

#endif /* BDCCPTable_hpp_ */
