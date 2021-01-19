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

#ifndef TypeManager_hpp_
#define TypeManager_hpp_

#include <table/TableInfo.hpp>
#include <string>
#include <map>

#include "QueryCompileException.hpp"

namespace pfabric {

class TypeManager {
public:
  typedef std::pair<TableInfo, std::string> TypeInfo;
  typedef std::map<std::string, TypeInfo> TableTypeMap;
  typedef TableTypeMap::const_iterator TypeIterator;

  TypeManager() {}

  void registerType(const TableInfo& tInfo);
  std::string nameOfType(const TableInfo& tInfo);

  TypeIterator begin() const { return mTypeTable.begin(); }
  TypeIterator end() const { return mTypeTable.end(); }

private:
  TableTypeMap mTypeTable;
};

}

#endif
