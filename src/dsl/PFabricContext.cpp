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

#include <sstream>

#include "dsl/PFabricContext.hpp"
#include "dsl/Topology.hpp"

using namespace pfabric;

PFabricContext::PFabricContext() {
}

PFabricContext::~PFabricContext() {
}

PFabricContext::TopologyPtr PFabricContext::createTopology() {
  return std::make_shared<Topology>();
}

TableInfoPtr PFabricContext::getTableInfo(const std::string& tblName) {
    // look for the table
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end()) {
      // if found then we return it
      return it->second->tableInfo();
    }
    else {
      // std::cout << "table not found: '" << tblName << "' : " << mTableSet.size() << std::endl;
      //return std::shared_ptr<TableInfo>();
      std::stringstream errMsg;
      errMsg << "table not found: '" << tblName << "' : " << mTableSet.size() << '\n';
      throw TableException(errMsg.str().c_str());
    }
  }
