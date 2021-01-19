/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef TableInfo_hpp_
#define TableInfo_hpp_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/Tuple.hpp"

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

using TableInfoPtr = std::shared_ptr<TableInfo>;

struct GetType {
  template<typename T>
    static auto apply(T &t) {
      return ColumnInfo::Void_Type;
    }
};

template<>
inline auto GetType::apply<int>(int &t) {
  return ColumnInfo::Int_Type;
}

template<>
inline auto GetType::apply<double>(double &t) {
  return ColumnInfo::Double_Type;
}

template<>
inline auto GetType::apply<std::string>(std::string &t) {
  return ColumnInfo::String_Type;
}

template<class Tuple, std::size_t CurrentIndex>
struct TupleTypes;

template<class Tuple, std::size_t CurrentIndex>
struct TupleTypes {
  static void apply(Tuple tp, std::vector<ColumnInfo> &cols) {
    TupleTypes<Tuple, CurrentIndex - 1>::apply(tp, cols);
    auto type = GetType::apply(std::get<CurrentIndex - 1>(tp));
    cols.push_back(ColumnInfo("", type));
  }
};

template<class Tuple>
struct TupleTypes<Tuple, 1> {
	static void apply(Tuple tp, std::vector<ColumnInfo> &cols) {
		auto type = GetType::apply(std::get<0>(tp));
		cols.push_back(ColumnInfo("", type));
	}
};

template<class Tuple>
TableInfo constructSchema(const std::string &tableName) {
	using Base = typename Tuple::Base;
	Base t; // create default initialized std::tuple

	std::vector<ColumnInfo> cols;
	TupleTypes<Base, std::tuple_size<Base>::value>::apply(t, cols);
	TableInfo tInfo(tableName);
	tInfo.setColumns(cols);
	return tInfo;
}

template<typename T>
struct is_tuple_impl : std::false_type {};
template<typename... Ts>
struct is_tuple_impl<pfabric::Tuple<Ts...>> : std::true_type {};
template<typename T>
struct is_tuple : is_tuple_impl<std::decay_t<T>> {};

}

std::ostream& operator<<(std::ostream& os, pfabric::ColumnInfo::ColumnType ct);

#endif
