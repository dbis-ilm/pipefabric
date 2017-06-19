#ifndef PTableInfo_hpp_
#define PTableInfo_hpp_

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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

class ColumnInfo {
public:
  enum ColumnType { Void_Type, Int_Type, Double_Type, String_Type };

  ColumnInfo(pool_base pop) : ColumnInfo(pop, "", ColumnType::Void_Type) {}

  ColumnInfo(pool_base pop, const std::string& n, ColumnType ct) : mColType(ct) {
    transaction::exec_tx(pop, [&] {
      mColName = make_persistent<char[]>(n.length() +1);
      strcpy(mColName.get(), n.c_str());
    });
  }

  const std::string getName() const { return mColName.get(); }

  const ColumnType getType() const { return mColType.get_ro(); }

  bool operator==(const ColumnInfo& other) const {
    return (strcmp(mColName.get(), other.mColName.get()) == 0 && mColType.get_ro() == other.mColType.get_ro());
  }

  bool operator<(const ColumnInfo& other) const {
    return strcmp(mColName.get(), other.mColName.get()) <= 0;
  }

private:
  persistent_ptr<char[]> mColName;
  p<ColumnType> mColType;
};

typedef std::initializer_list<std::pair<std::string, ColumnInfo::ColumnType>> ColumnInitList;

/** TODO: For later, template implementation
template< bool isFixedSize >
struct IsFixedSize {
  static const bool IS_FIXED_SIZE = isFixedSize;
};

template < ColumnInfo::ColumnType ColType >
struct ColTypeTraits;

template <>
struct ColTypeTraits< ColumnInfo::Int_Type > : IsFixedSize<true> {
  static const std::size_t COLUMN_SIZE = sizeof(int32_t);
};

template <>
struct ColTypeTraits< ColumnInfo::Double_Type > : IsFixedSize<true> {
  static const std::size_t COLUMN_SIZE = sizeof(double);
};

template <>
struct ColTypeTraits< ColumnInfo::String_Type > : IsFixedSize<false>{};

using Offset = std::size_t;

template< ColumnInfo::ColumnType ColType, typename Col >
typename std::enable_if< not ColTypeTraits< ColType >::IS_FIXED_SIZE, Offset >::type getSize(const  Col& t ) {
  //TODO: runtime size calculation
}

template< ColumnInfo::ColumnType ColType, typename Col >
typename std::enable_if< ColTypeTraits< ColType >::IS_FIXED_SIZE, Offset >::type getSize(const Col& t ) {
  using ColTraits = ColTypeTraits<ColType>;
  return ColTraits::COLUMN_SIZE;
}

*/

class PTableInfo {
 public:
  typedef persistent_ptr<PTableInfo> TableInfoPtr;
  typedef std::vector<ColumnInfo, nvml::obj::allocator<ColumnInfo>> ColumnVector;
  typedef ColumnVector::const_iterator ColumnIterator;

  PTableInfo(){}

  PTableInfo(const std::string& name, ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type)
    : mName(name.c_str()), mKeyType(keyType) {}

  PTableInfo(const std::string& name, ColumnInitList columns,
      ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type) :
      mName(name.c_str()), mKeyType(keyType) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      mColumns = make_persistent<ColumnVector>();
      for (const auto &c : columns)
        mColumns->push_back(ColumnInfo(pop, c.first, c.second));
    });
  }

  PTableInfo(const std::string& name, const ColumnVector& columns,
        ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type) :
        mName(name.c_str()), mKeyType(keyType) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      mColumns = make_persistent<ColumnVector>(columns.begin(), columns.end());
    });
  }

  const std::string tableName() const { return std::string(mName.get()); }

  std::string typeSignature() const;

  std::string generateTypeDef() const;

  ColumnInfo::ColumnType typeOfKey() const { return mKeyType.get_ro(); }

  int findColumnByName(const std::string& colName) const;

  const ColumnInfo& columnInfo(int pos) const { return mColumns->at(pos); }

  const std::size_t numColumns() const { return mColumns->size(); }

  void setColumns(const ColumnVector vec) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      delete_persistent<ColumnVector>(mColumns);
      mColumns = make_persistent<ColumnVector>(vec.cbegin(), vec.cend());
    });
   }

  TableInfoPtr makeShared() const {
    auto pop = pool_by_vptr(this);
    TableInfoPtr tInfo_ptr = nullptr;
    transaction::exec_tx(pop, [&] {
       tInfo_ptr = make_persistent<PTableInfo>(std::string(mName.get()), *mColumns, mKeyType.get_ro());
    });
    return tInfo_ptr;
  }

  ColumnIterator begin() const { return mColumns->begin(); }
  ColumnIterator end() const { return mColumns->end(); }

 private:
  persistent_ptr<const char[]> mName;
  persistent_ptr<ColumnVector> mColumns;
  p<ColumnInfo::ColumnType> mKeyType;
};

}
typedef persistent_ptr<nvm::PTableInfo> TableInfoPtr;

} /* namespace pfabric::nvm */

std::ostream& operator<<(std::ostream& os, pfabric::nvm::ColumnInfo::ColumnType ct);

namespace std {
  template <>
  struct hash<pfabric::nvm::ColumnInfo> {
    std::size_t operator()(const pfabric::nvm::ColumnInfo& c) const {
      return std::hash<std::string>()(c.getName());
    }
  };
}

#endif
