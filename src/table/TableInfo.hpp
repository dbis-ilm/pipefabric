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
};

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

 private:
  std::string mName;
  std::vector<ColumnInfo> mColumns;
  ColumnInfo::ColumnType mKeyType;
};

typedef std::shared_ptr<TableInfo> TableInfoPtr;
}

std::ostream& operator<<(std::ostream& os, pfabric::ColumnInfo::ColumnType ct);

#endif
