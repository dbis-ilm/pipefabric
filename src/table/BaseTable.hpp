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

#ifndef BaseTable_hpp_
#define BaseTable_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>

#include <mutex>
#include <shared_mutex>

#include <boost/signals2.hpp>
#include "table/TableInfo.hpp"
#include "table/TableException.hpp"

namespace pfabric {

/**
 * Constants representing different modes for working with a table.
 */
struct TableParams {
  /**
   * NotificationMode specifies when a stream tuple is produced
   * from the table.
   */
  enum NotificationMode {
    Immediate, //< directly for each updated tuple
    OnCommit   //< on transaction commit
  };

  /**
   * ModificationMode describes the kind of modification that
   * triggered the tuple.
   */
  enum ModificationMode {
    Insert,  //< tuple was inserted into the table
    Update,  //< tuple was updated
    Delete   //< tuple was deleted
  };
};

/**
 * @brief BaseTable is the abstract base class for all table objects.
 */
class BaseTable {
protected:
  BaseTable() {}

  /**
   * Constructor for creating an empty table with a given schema.
   */
  BaseTable(const TableInfo& tInfo) : mTableInfo(std::make_shared<TableInfo>(tInfo)) {}

public:
  virtual ~BaseTable() {}

  /**
   * Return a pointer to the TableInfo object describing the schema of the table.
   *
   * @return a pointer to the corresponding TableInfo object
   */
  TableInfoPtr tableInfo() { return mTableInfo; }

protected:
  TableInfoPtr mTableInfo;    //< explicit schema information (can be empty)
};

}

#endif

