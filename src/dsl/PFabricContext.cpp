/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */
#include <string>
#include "PFabricContext.hpp"

using namespace pfabric;

PFabricContext::PFabricContext() {
}

PFabricContext::~PFabricContext() {}

PFabricContext::TopologyPtr PFabricContext::createTopology() {
  return std::make_shared<Topology>();
}

bool PFabricContext::tableExists(const std::string& tblName) const {
    // look for the table
    auto it = mTableSet.find(tblName);
    return (it != mTableSet.end()); 
}

TableInfoPtr PFabricContext::getTableInfo(const std::string& tblName) {
    // look for the table
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end()) {
      // if found then we return it
      return it->second->tableInfo();
    }
    else {
      // TODO: shouldn't we throw an exception here???
      std::cout << "table not found: '" << tblName << "' : " << mTableSet.size() << std::endl;
      return std::shared_ptr<TableInfo>();
    }
  }
