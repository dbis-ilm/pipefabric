#ifndef PTableInfo_hpp_
#define PTableInfo_hpp_

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "table/TableInfo.hpp"
#include "nvm/PString.hpp"

#include "nvml/include/libpmemobj++/allocator.hpp"
#include "nvml/include/libpmemobj++/detail/persistent_ptr_base.hpp"
#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/make_persistent_array.hpp"
#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/pool.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/transaction.hpp"
#include "nvml/include/libpmemobj++/utils.hpp"

using nvml::obj::delete_persistent;
using nvml::obj::make_persistent;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::pool_base;
using nvml::obj::pool_by_vptr;
using nvml::obj::transaction;

namespace pfabric { namespace nvm {

class PColumnInfo {
public:
  PColumnInfo(pool_base pop) : PColumnInfo(pop, "", ColumnInfo::ColumnType::Void_Type) {}

  PColumnInfo(pool_base pop, const std::string& n, ColumnInfo::ColumnType ct) : mColType(ct) {
    transaction::exec_tx(pop, [&] {
      mColName = make_persistent<char[]>(n.length() +1);
      strcpy(mColName.get(), n.c_str());
    });
  }

  const std::string getName() const { return mColName.get(); }

  const ColumnInfo::ColumnType getType() const { return mColType.get_ro(); }

private:
  persistent_ptr<char[]> mColName;
  p<ColumnInfo::ColumnType> mColType;
};

using ColumnInitList = std::initializer_list<std::pair<std::string, ColumnInfo::ColumnType>>;

class PTableInfo {
 public:
  using PTableInfoPtr = persistent_ptr<PTableInfo>;
  using ColumnVector = std::vector<PColumnInfo, nvml::obj::allocator<PColumnInfo>>;
  using ColumnIterator = ColumnVector::const_iterator;

  PTableInfo(){}

  PTableInfo(const TableInfo& _tInfo, ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type) : mKeyType(keyType){
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      mName.set(const_cast<std::string*>(& _tInfo.tableName()));
      mColumns = make_persistent<ColumnVector>();
      for (const auto &c : _tInfo)
        mColumns->push_back(PColumnInfo(pop, c.getName(), c.getType()));
    });
  }

  PTableInfo(const std::string& name, ColumnInitList columns,
             ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type) : mKeyType(keyType) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      mName.set(const_cast<std::string*>(&name));
      mColumns = make_persistent<ColumnVector>();
      for (const auto &c : columns)
        mColumns->push_back(PColumnInfo(pop, c.first, c.second));
    });
  }

  PTableInfo(const std::string& name, const ColumnVector& columns,
             ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type) : mKeyType(keyType) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      mName.set(const_cast<std::string*>(&name));
      mColumns = make_persistent<ColumnVector>(columns.begin(), columns.end());
    });
  }

  const std::string tableName() const { return std::string(mName.data()); }

  std::string typeSignature() const;

  std::string generateTypeDef() const;

  ColumnInfo::ColumnType typeOfKey() const { return mKeyType.get_ro(); }

  int findColumnByName(const std::string& colName) const;

  const PColumnInfo& columnInfo(int pos) const { return mColumns->at(pos); }

  const std::size_t numColumns() const { return mColumns->size(); }

  void setColumns(const ColumnVector vec) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      delete_persistent<ColumnVector>(mColumns);
      mColumns = make_persistent<ColumnVector>(vec.cbegin(), vec.cend());
    });
   }

  PTableInfoPtr makeShared() const {
    auto pop = pool_by_vptr(this);
    PTableInfoPtr tInfo_ptr = nullptr;
    transaction::exec_tx(pop, [&] {
       tInfo_ptr = make_persistent<PTableInfo>(std::string(mName.data()), *mColumns, mKeyType.get_ro());
    });
    return tInfo_ptr;
  }

  ColumnIterator begin() const { return mColumns->begin(); }
  ColumnIterator end() const { return mColumns->end(); }

// private:
  PString mName;
  persistent_ptr<ColumnVector> mColumns;
  p<ColumnInfo::ColumnType> mKeyType;
};

}
using PTableInfoPtr = persistent_ptr<nvm::PTableInfo>;

} /* namespace pfabric::nvm */

namespace std {
  template <>
  struct hash<pfabric::nvm::PColumnInfo> {
    std::size_t operator()(const pfabric::nvm::PColumnInfo& c) const {
      return std::hash<std::string>()(c.getName());
    }
  };
}

#endif
