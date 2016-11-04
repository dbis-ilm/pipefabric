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

struct TableParams {
  /**
   * NotificationMode specifies when a stream tuple is produced
   + from the table.
   */
  enum NotificationMode {
    Immediate, //< directly for each updated tuple
    OnCommit   //< on transaction commit
  };

  enum ModificationMode {
    Insert, Update, Delete
  };
};

class BaseTable {
protected:
  BaseTable() {}

  virtual ~BaseTable() {}
};

template <typename RecordType, typename KeyType = DefaultKeyType>
class Table : public BaseTable {
public:
  typedef std::unordered_map<KeyType, RecordType> TableMap;

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

  std::pair<TableIterator, TableIterator> select(Predicate func) {
    return make_pair(makeFilterIterator(mDataTable.begin(), func),
                      makeFilterIterator(mDataTable.end(), func));
  }

  std::pair<TableIterator, TableIterator> select() {
    auto alwaysTrue = [](const RecordType&) { return true; };
    return make_pair(makeFilterIterator(mDataTable.begin(), alwaysTrue),
                      makeFilterIterator(mDataTable.end(), alwaysTrue));
  }

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
