/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#include "TypeManager.hpp"
#include "QueryCompileException.hpp"
#include "UniqueNameGenerator.hpp"
#include "table/TableInfo.hpp"

using namespace pfabric;

void TypeManager::registerType(const TableInfo& tInfo) {
  auto sig = tInfo.typeSignature();
  if (mTypeTable.count(sig) == 0) {
    std::string typeName = UniqueNameGenerator::instance()->uniqueName("Tuple") + "_Type_";
    mTypeTable[sig] = std::make_pair(tInfo, typeName);
  }
}

std::string TypeManager::nameOfType(const TableInfo& tInfo) {
  auto iter = mTypeTable.find(tInfo.typeSignature());
  if (iter == mTypeTable.end()) 
    throw QueryCompileException("unkown type");

  auto typeInfo = iter->second;
  return typeInfo.second;
}

