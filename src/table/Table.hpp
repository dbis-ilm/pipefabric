#ifndef Table_hpp_
#define Table_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>

#include <boost/signals2.hpp>

namespace pfabric {

class TableException : public std::exception {
    virtual const char* what() const throw() {
      return "TableException";
    }
};

struct TableParams {
  enum NotificationMode {
    Immediate, OnCommit
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
  typedef std::function<bool(const RecordType&)> Predicate;
  typedef std::function<RecordType(const RecordType&)> UpdaterFunc;

  typedef boost::signals2::signal<void (const RecordType&, TableParams::ModificationMode)> ObserverCallback;

  Table() {}

    ~Table() { std::cout << "deallocate table" << std::endl; }

  void insert(KeyType key, const RecordType& rec) throw (TableException) {
    mDataTable.insert({key, rec});
    notifyObservers(rec, TableParams::Insert, TableParams::Immediate);
  }

  unsigned long deleteByKey(KeyType key) {
    if (!mImmediateObservers.empty()) {
      auto res = mDataTable.find(key);
      if (res != mDataTable.end())
        notifyObservers(res->second, TableParams::Delete, TableParams::Immediate);
    }
    return mDataTable.erase(key);
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
    return 0;
  }

  unsigned long updateWhere(Predicate pfunc, UpdaterFunc ufunc) {
    return 0;
  }

  const RecordType& getByKey(KeyType key) throw (TableException) {
    auto res = mDataTable.find(key);
    if (res != mDataTable.end())
      return res->second;
    else
      throw TableException();
  }

  // TODO: TableIterator select(Predicate func);

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

   std::unordered_map<KeyType, RecordType> mDataTable;
   ObserverCallback mImmediateObservers, mDeferredObservers;
};

}

#endif
