#ifndef TableInfo_hpp_
#define TableInfo_hpp_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace pfabric {

struct ColumnInfo {
  enum ColumnType { Void_Type, Int_Type, Double_Type, String_Type };

  ColumnInfo(const std::string& n, ColumnType ct) : mColName(n), mColType(ct) {}

  std::string mColName;
  ColumnType mColType;

  bool operator==(const ColumnInfo& other) const {
    return (mColName == other.mColName && mColType == other.mColType);
  }

  bool operator<(const ColumnInfo& other) const {
    return mColName < other.mColName;
  }
};


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

class TableInfo {
 public:
  typedef std::vector<ColumnInfo>::const_iterator ColumnIterator;

  TableInfo() {}

  TableInfo(const std::string& name, std::initializer_list<ColumnInfo> columns,
            ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type)
      : mName(name), mColumns(columns), mKeyType(keyType) {}

  const std::string& tableName() const { return mName; }

  std::string typeSignature() const;

  std::string generateTypeDef() const;

  ColumnInfo::ColumnType typeOfKey() const { return mKeyType; }

  int findColumnByName(const std::string& colName) const;
  const ColumnInfo& columnInfo(int pos) const { return mColumns[pos]; }

  void setColumns(const std::vector<ColumnInfo>& vec) { mColumns = vec; }

  ColumnIterator begin() const { return mColumns.begin(); }
  ColumnIterator end() const { return mColumns.end(); }

  std::size_t numColumns() const { return mColumns.size(); }

 private:
  std::string mName;
  std::vector<ColumnInfo> mColumns;
  ColumnInfo::ColumnType mKeyType;
};

typedef std::shared_ptr<TableInfo> TableInfoPtr;
}

std::ostream& operator<<(std::ostream& os, pfabric::ColumnInfo::ColumnType ct);

namespace std {
  template <>
  struct hash<pfabric::ColumnInfo> {
    std::size_t operator()(const pfabric::ColumnInfo& c) const {
      return std::hash<std::string>()(c.mColName);
    }
  };
}

#endif
