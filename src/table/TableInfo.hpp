/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef TableInfo_hpp_
#define TableInfo_hpp_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace pfabric {

struct ColumnInfo {
  enum ColumnType { Void_Type, Int_Type, Double_Type, String_Type, UInt_Type };

  ColumnInfo(const std::string& n, ColumnType ct) : mColName(n), mColType(ct) {}

  const std::string& getName() const { return mColName; }

  const ColumnType& getType() const { return mColType; }

private:
  std::string mColName;
  ColumnType mColType;
};

class TableInfo {
 public:
  using ColumnVector = std::vector<ColumnInfo>;
  using ColumnIterator = ColumnVector::const_iterator;

  TableInfo() {}

  TableInfo(const std::string& name, std::initializer_list<ColumnInfo> columns,
            ColumnInfo::ColumnType keyType = ColumnInfo::Void_Type)
      : mName(name), mColumns(columns), mKeyType(keyType) {}

  TableInfo(const std::string& name) : mName(name), mColumns(), mKeyType(ColumnInfo::Void_Type) {}

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
