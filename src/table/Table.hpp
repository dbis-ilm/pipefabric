#ifndef Table_hpp_
#define Table_hpp_

#include <iostream>
#include <unordered_map>
#include <functional>
#include <exception>

namespace pfabric {

class TableException : public std::exception {
    virtual const char* what() const throw() {
      return "TableException";
    }
};


template <typename RecordType, typename KeyType = DefaultKeyType>
class Table {
public:
  typedef std::function<bool(const RecordType&)> Predicate;
  typedef std::function<RecordType(const RecordType&)> UpdaterFunc;

  enum NotificationMode {
    Immediate, OnCommit
  };

  Table() {}

  ~Table() {}

  void insert(KeyType key, const RecordType& rec) throw (TableException) {
    mDataTable.insert({key, rec});
  }

  unsigned long deleteByKey(KeyType key) {
    return mDataTable.erase(key);
  }
  unsigned long deleteWhere(Predicate func) {
    unsigned long num = 0;
    for(auto it = mDataTable.begin(); it != mDataTable.end(); ) {
      if (func(it->second)) {
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

private:
   std::unordered_map<KeyType, RecordType> mDataTable;
};

}

#endif
