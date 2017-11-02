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

#ifndef RDBTable_hpp_
#define RDBTable_hpp_

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
#include "table/TableException.hpp"
#include "table/TableInfo.hpp"

namespace pfabric {

namespace detail {
template <class T>
inline rocksdb::Slice valToSlice(const T& t) {
  return rocksdb::Slice(reinterpret_cast<const char*>(&t), sizeof(t));
}

template <class T>
inline T& sliceToVal(const rocksdb::Slice& slice) {
  return *(reinterpret_cast<T*>(const_cast<char*>(slice.data())));
}

template <class T>
inline T* sliceToTuplePtr(const rocksdb::Slice& slice) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(slice.data());
  StreamType buf(ptr, ptr + slice.size());
  return new T(buf);
}

template <class T>
inline T sliceToTuple(const rocksdb::Slice& slice) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(slice.data());
  StreamType buf(ptr, ptr + slice.size());
  return T(buf);
}
}

template <typename RecordType>
class RDBTableIterator {
  //  inline RecordType* fromSlice(const rocksdb::Slice& s) {
  //    return pfabric::detail::sliceToTuplePtr<RecordType>(s);
  //  }

 public:
  typedef std::function<bool(const RecordType&)> Predicate;
  typedef SmartPtr<RecordType> RecordTypePtr;

  explicit RDBTableIterator() {}
  explicit RDBTableIterator(rocksdb::Iterator* i, Predicate p) : pred(p) {
    iter.reset(i);
    iter->SeekToFirst();
    // make sure the initial iterator position refers to an entry satisfying
    // the predicate
    while (iter->Valid() &&
           !pred(pfabric::detail::sliceToTuple<RecordType>(iter->value())))
      iter->Next();
  }

  RDBTableIterator operator++() {
    iter->Next();
    while (iter->Valid() &&
           !pred(pfabric::detail::sliceToTuple<RecordType>(iter->value())))
      iter->Next();
    return *this;
  }

  RDBTableIterator operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool isValid() const { return iter->Valid(); }
  RecordTypePtr operator*() {
    SmartPtr<RecordType> tptr;
    tptr.reset(pfabric::detail::sliceToTuplePtr<RecordType>(iter->value()));
    return tptr;
  }
  //  RecordType* operator->() { return &fromSlice(iter->value()); }

 protected:
  std::shared_ptr<rocksdb::Iterator> iter;
  Predicate pred;
};

template <typename RecordType>
inline RDBTableIterator<RecordType> makeRDBTableIterator(
    rocksdb::Iterator* i, typename RDBTableIterator<RecordType>::Predicate p) {
  return RDBTableIterator<RecordType>(i, p);
}

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
class RDBTable : public BaseTable {
 public:
  //< typedef for a predicate evaluated using a scan
  // typedef std::function<bool(const RecordType&)> Predicate;

  //< typedef for a updater function which returns a modification of the
  // parameter tuple
  typedef std::function<void(RecordType&)> UpdaterFunc;

  //< typedefs for a function performing updates + deletes. Similar to
  // UpdaterFunc
  //< it allows to update the tuple, but also to delete it (indictated by the
  //< setting the bool component of @c UpdateResult to false)
  typedef std::function<bool(RecordType&)> UpdelFunc;

  typedef std::function<RecordType()> InsertFunc;

  //< typedef for a callback function which is invoked when the table was
  // updated
  typedef boost::signals2::signal<void(const RecordType&,
                                       TableParams::ModificationMode)>
      ObserverCallback;

  //< typedef for an iterator to scan the table
  typedef RDBTableIterator<RecordType> TableIterator;

  //< typedef for a predicate evaluated using a scan: see @TableIterator for
  // details
  typedef typename TableIterator::Predicate Predicate;

  RDBTable(const TableInfo& tInfo) throw(TableException)
      : BaseTable(tInfo), mTableName(tInfo.tableName()) {
    openOrCreateTable(tInfo.tableName());
  }

  /**
   * Constructor for creating an empty table.
   */
  RDBTable(const std::string& tableName) throw(TableException)
      : mTableName(tableName) {
    openOrCreateTable(tableName);
  }

  /**
    * Destructor for table.
    */
  ~RDBTable() {
    if (db != nullptr) {
      delete db;
    }
  }

  /**
   * Constructor for creating an empty table with a given schema.
   */
  // Table(const TableInfo& tInfo) : BaseTable(tInfo) {}

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
  void insert(KeyType key, const RecordType& rec) throw(TableException) {
    {
      StreamType buf;
      rec.serializeToStream(buf);
      auto status =
          db->Put(writeOptions, pfabric::detail::valToSlice(key),
                  rocksdb::Slice(reinterpret_cast<const char*>(buf.data()),
                                 buf.size()));
      if (status.ok()) numRecords++;
      // TODO: what happens in case of replacing a key???
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
      auto keySlice = pfabric::detail::valToSlice(key);
      std::string res;
      auto status = db->Get(readOptions, keySlice, &res);
      if (status.ok()) {
        // if the key exists: notify our observers
        notifyObservers(pfabric::detail::sliceToTuple<RecordType>(
                            rocksdb::Slice(res.data(), res.size())),
                        TableParams::Delete, TableParams::Immediate);
        // and delete the tuples
        status = db->Delete(writeOptions, keySlice);
        nres = status.ok() ? 1 : 0;
      }
    }
    numRecords -= nres;
    return nres;
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
    unsigned long num = 0;
    // we perform a full scan here ...
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      auto tup = pfabric::detail::sliceToTuple<RecordType>(it->value());
      // and check the predicate
      if (func(tup)) {
        notifyObservers(tup, TableParams::Delete, TableParams::Immediate);
        db->Delete(writeOptions, it->key());
        num++;
      }
    }
    delete it;
    numRecords -= num;
    return num;
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
    auto keySlice = pfabric::detail::valToSlice(key);
    std::string resultData;
    auto status = db->Get(readOptions, keySlice, &resultData);
    if (status.ok()) {
      TableParams::ModificationMode mode = TableParams::Update;
      unsigned long num = 1;

      // if the key exists: notify our observers
      auto rec = pfabric::detail::sliceToTuple<RecordType>(
          rocksdb::Slice(resultData.data(), resultData.size()));
      // perform the update
      auto res = ufunc(rec);

      // check whether we have to perform an update ...
      if (res) {

        StreamType buf;
        rec.serializeToStream(buf);
        status =
            db->Put(writeOptions, keySlice,
                    rocksdb::Slice(reinterpret_cast<const char*>(buf.data()),
                                   buf.size()));
      } else {
        // or a delete
        status = db->Delete(writeOptions, keySlice);
        num = status.ok() ? 1 : 0;
        mode = TableParams::Delete;
      }
      // notify the observers
      notifyObservers(rec, mode, TableParams::Immediate);
      return num;
    }
    else {
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
   * The actual modification is done by the updater function specified as
   * parameter.
   *
   * @param key the key of the tuple to be modified
   * @param func a function performing the modification by returning a modified
   *        tuple
   * @return the number of modified tuples
   */
  unsigned long updateByKey(KeyType key, UpdaterFunc ufunc) {
    auto keySlice = pfabric::detail::valToSlice(key);
    std::string resultData;
    auto status = db->Get(readOptions, keySlice, &resultData);
    if (status.ok()) {
      // if the key exists: notify our observers
      auto rec = pfabric::detail::sliceToTuple<RecordType>(
          rocksdb::Slice(resultData.data(), resultData.size()));
      ufunc(rec);
      StreamType buf;
      rec.serializeToStream(buf);
      auto status =
          db->Put(writeOptions, keySlice,
                  rocksdb::Slice(reinterpret_cast<const char*>(buf.data()),
                                 buf.size()));
      notifyObservers(rec, TableParams::Update, TableParams::Immediate);
      return 1;
    }
    return 0;
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
    unsigned long num = 0;
    // we perform a full table scan
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      auto tup = pfabric::detail::sliceToTuple<RecordType>(it->value());
      // and check the predicate
      if (pfunc(tup)) {
        ufunc(tup);
        StreamType buf;
        tup.serializeToStream(buf);
        auto status =
            db->Put(writeOptions, it->key(),
                    rocksdb::Slice(reinterpret_cast<const char*>(buf.data()),
                                   buf.size()));
        notifyObservers(tup, TableParams::Update, TableParams::Immediate);
      }
    }
    delete it;
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
  SmartPtr<RecordType> getByKey(KeyType key) throw(TableException) {
    std::string resultData;
    auto status =
        db->Get(readOptions, pfabric::detail::valToSlice(key), &resultData);
    if (status.ok()) {
      // if we found the tuple we just return it
      SmartPtr<RecordType> tptr;
      tptr.reset(pfabric::detail::sliceToTuplePtr<RecordType>(
          rocksdb::Slice(resultData.data(), resultData.size())));
      return tptr;
    } else
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
  TableIterator select(Predicate func) {
    return makeRDBTableIterator<RecordType>(
        db->NewIterator(rocksdb::ReadOptions()), func);
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
  TableIterator select() {
    auto alwaysTrue = [](const RecordType&) { return true; };
    return makeRDBTableIterator<RecordType>(
        db->NewIterator(rocksdb::ReadOptions()), alwaysTrue);
  }

  /**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   */
  unsigned long size() const { return numRecords; }

  void drop() {
    delete db;
    db = nullptr;
    boost::filesystem::path dbFile(mTableName + ".db");
    boost::filesystem::remove_all(dbFile);
  }

  /**
   * @brief Register an observer
   *
   * Registers an observer (a slot) which is notified in case of updates on the
   * table.
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

  rocksdb::DB* _db() { return db; }

 private:
  void openOrCreateTable(const std::string& tableName) throw(TableException) {
    std::string fileName = tableName + ".db";
    rocksdb::Options options;
    options.create_if_missing = true;

    rocksdb::Status status = rocksdb::DB::Open(options, fileName, &db);
    if (!status.ok()) {
      throw TableException(status.ToString().c_str());
    }
    updateRecordCounter();
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
  void notifyObservers(const RecordType& rec,
                       TableParams::ModificationMode mode,
                       TableParams::NotificationMode notify) {
    if (notify == TableParams::Immediate) {
      mImmediateObservers(rec, mode);
    } else {
      // TODO: implement defered notification
      mDeferredObservers(rec, mode);
    }
  }

  void updateRecordCounter() {
    numRecords = 0;
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());

    for (it->SeekToFirst(); it->Valid(); it->Next()) numRecords++;

    delete it;
  }

  std::string mTableName;
  rocksdb::DB* db;
  rocksdb::WriteOptions writeOptions;
  rocksdb::ReadOptions readOptions;
  ObserverCallback mImmediateObservers, mDeferredObservers;
  unsigned long numRecords;
};
}

#endif
