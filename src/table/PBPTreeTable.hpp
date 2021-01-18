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

#ifndef PBPTreeTable_hpp_
#define PBPTreeTable_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>
#include <cstdint>
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

#include "pbptrees/PBPTree.hpp"

#include "pfabric_config.h"
#include "core/Tuple.hpp"
#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {

constexpr auto BRANCHSIZE = 32;
constexpr auto LEAFSIZE = 16;

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::transaction;
using dbis::pbptrees::PBPTree;

template<typename KeyType, typename RecordType>
class PBPTreeIterator {
 public:
  static_assert(is_tuple<RecordType>::value, "Value type must be a pfabric::Tuple");
  using TupleType = typename RecordType::Base;
  using Predicate = std::function<bool(const RecordType &)>;
  using PBTreeType = PBPTree<KeyType, TupleType, BRANCHSIZE, LEAFSIZE>;

  explicit PBPTreeIterator() {
  }

  explicit PBPTreeIterator(typename PBTreeType::iterator &&_iter, typename PBTreeType::iterator &&_end, Predicate _pred) :
    iter(std::move(_iter)), end(std::move(_end)), pred(_pred) {

    while (isValid() && !pred((*iter).second))
      ++iter;
  }

  PBPTreeIterator &operator++() {
    ++iter;
    while (isValid() && !pred((*iter).second))
      ++iter;
    return *this;
  }

  PBPTreeIterator operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool isValid() const {
    return iter != end;
  }

  SmartPtr<RecordType> operator*() {
    SmartPtr<RecordType> tptr(new RecordType((*iter).second));
    return tptr; //TODO: Is this to expensive?
  }

 protected:
  // PTable Iterator
  typename PBTreeType::iterator iter, end;
  // Selection predicate
  Predicate pred;

};

template<typename KeyType, typename RecordType>
inline PBPTreeIterator<KeyType, RecordType> makePBPTreeIterator(
  typename PBPTree<KeyType, typename RecordType::Base, BRANCHSIZE, LEAFSIZE>::iterator &&iter,
  typename PBPTree<KeyType, typename RecordType::Base, BRANCHSIZE, LEAFSIZE>::iterator &&end,
  typename PBPTreeIterator<KeyType, RecordType>::Predicate pred) {
  return PBPTreeIterator<KeyType, RecordType>(std::move(iter), std::move(end), pred);
}

/**************************************************************************//**
 * \brief PBPTreeTable is a class for storing a relation of tuples of the same type.
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
class PBPTreeTable : public BaseTable {
 public:
  static_assert(is_tuple<RecordType>::value, "Value type must be a pfabric::Tuple");
  using TupleType = typename RecordType::Base;
  using PBTreeType = PBPTree<KeyType, TupleType, BRANCHSIZE, LEAFSIZE>;

  struct root {
    persistent_ptr<PBTreeType> btree;
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
  using TableIterator = PBPTreeIterator<KeyType, RecordType>;

  /** typedef for a predicate evaluated using a scan: see \TableIterator for details */
  using Predicate = typename TableIterator::Predicate;

  /************************************************************************//**
   * \brief Constructor for creating an empty table with only a given name.
   *****************************************************************************/
  explicit PBPTreeTable(const std::string &tableName) : BaseTable(constructSchema<RecordType>(tableName)) {
    openOrCreateTable(constructSchema<RecordType>(tableName));
  }

  /************************************************************************//**
   * \brief Constructor for creating an empty table with a given schema.
   *****************************************************************************/
  explicit PBPTreeTable(const TableInfo &tInfo) :
    BaseTable(tInfo) {
    openOrCreateTable(tInfo);
  }

  /************************************************************************//**
   * \brief Destructor for table.
   *****************************************************************************/
  ~PBPTreeTable() {
    // pop.close();
  }

  /************************************************************************//**
   * \brief Insert a tuple.
   *
   * Insert or update the given tuple \rec with the given key into the table.
   * After the insert all observers are notified.
   *
   * \param key the key value of the tuple
   * \param rec the actual tuple
   *****************************************************************************/
  void insert(const KeyType key, const RecordType &rec) noexcept(false) {
    TupleType *tptr;
    if (btree->lookupRef(key, &tptr)) {
      *tptr = rec.data();
      notifyObservers(rec, TableParams::Update, TableParams::Immediate);
    }
    else {
      btree->insert(key, rec.data());
      notifyObservers(rec, TableParams::Insert, TableParams::Immediate);
    }
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
    return btree->erase(key);
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
  const SmartPtr<RecordType> getByKey(const KeyType key) const {
    TupleType tt;
    if(btree->lookup(key, &tt)) {
      return new RecordType(tt);
    }
    else throw TableException("Key not found: " + key);
  }

  const bool getByKey(const KeyType key, SmartPtr<RecordType> &outValue) const {
    TupleType tt;
    if (btree->lookup(key, &tt)) {
      outValue.reset(new RecordType(tt));
      return true;
    }
    return false;
  }

  bool getAsRef(const KeyType key, TupleType **val) const {
    return btree->lookupRef(key, val);
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
    return makePBPTreeIterator<KeyType, RecordType>(std::move(btree->begin()), std::move(btree->end()), func);
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
    return makePBPTreeIterator<KeyType, RecordType>(std::move(btree->begin()), std::move(btree->end()), alwaysTrue);
  }

  /************************************************************************//**
   * \brief Return the number of tuples stored in the table.
   *
   * \return the number of tuples
   *****************************************************************************/
  unsigned long size() const {
    //TODO: Maybe there is a more efficient way
    const auto cnt = std::distance(btree->begin(), btree->end());
    //for(const auto _: *btree) ++cnt;
    return cnt;
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
    //auto pop = pool_by_pptr(q);
    transaction::run(pop, [&] {
      delete_persistent<PBTreeType>(q->btree);
      q->btree = nullptr;
      delete_persistent<root>(q);
      q = nullptr;
    });
    pop.close();
    //pmempool_rm((pfabric::gPmemPath + BaseTable::mTableInfo->tableName() + ".db").c_str(), 1);
    std::remove((BaseTable::mTableInfo->tableName()+".db").c_str());
  }

  void truncate() {
    pobj_alloc_class_desc alloc_class = pop.template ctl_get<struct pobj_alloc_class_desc>("heap.alloc_class.128.desc");
    transaction::run(pop, [&] {
      delete_persistent<PBTreeType>(q->btree);
      q->btree = make_persistent<PBTreeType>(alloc_class);
      btree = q->btree;
    });

  }

  void print() {
    btree->print(false);
  }

  persistent_ptr<struct root> q;
  persistent_ptr<PBTreeType> btree;

 private:
  pool<root> pop;

  void openOrCreateTable(const TableInfo &tableInfo) noexcept(false) {
    const std::string path = pfabric::gPmemPath + tableInfo.tableName() + ".db";

    if (access(path.c_str(), F_OK) != 0) {
      //TODO: How do we estimate the required pool size
      pop = pool<root>::create(path, "PBPTree", pfabric::gPmemPoolSize);
      pobj_alloc_class_desc alloc_class = pop.ctl_set("heap.alloc_class.128.desc",
                                                      PBTreeType::AllocClass);
      transaction::run(pop, [&] {
        pop.root()->btree = make_persistent<PBTreeType>(alloc_class);
      });
    } else {
      pop = pool<root>::open(path, "PBPTree");
      pobj_alloc_class_desc alloc_class = pop.ctl_set("heap.alloc_class.128.desc",
                                                      PBTreeType::AllocClass);
    }
    q = std::move(pop.root());
    btree = (q->btree); ///< retrieve volatile address of btree
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

  ObserverCallback mImmediateObservers, mDeferredObservers;

}; /* class PBPTreeTable */

} /* namespace pfabric */

#endif /* PBPTreeTable_hpp_ */
