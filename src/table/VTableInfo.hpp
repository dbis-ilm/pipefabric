#ifndef VTableInfo_hpp_
#define VTableInfo_hpp_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace pfabric {

class ColumnInfo {
public:
  enum ColumnType { Void_Type, Int_Type, Double_Type, String_Type };

  ColumnInfo(const std::string& n, ColumnType ct) : mColName(n), mColType(ct) {}

  std::string getName() { return mColName; }

  ColumnType getType() { return mColType; }

private:
  std::string mColName;
  ColumnType mColType;
};

class VTableInfo {
 public:
  typedef std::vector<ColumnInfo>::const_iterator ColumnIterator;

  VTableInfo() {}

  VTableInfo(const std::string& name, std::initializer_list<ColumnInfo> columns,
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

typedef std::shared_ptr<VTableInfo> TableInfoPtr;
}

std::ostream& operator<<(std::ostream& os, pfabric::ColumnInfo::ColumnType ct);

#endif
