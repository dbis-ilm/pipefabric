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
#ifndef PFabricContext_hpp_
#define PFabricContext_hpp_

#include <string>
#include <map>

#include "core/PFabricTypes.hpp"
#include "table/Table.hpp"
#include "topology/Topology.hpp"

namespace pfabric {

/**
 * @brief PFabricContext provides the main entry point to PipeFabric objects.
 *
 * PFabricContext represents the context object for creating and managing
 * PipeFabric objects such as dataflow programs/queries and tables. It is
 * used to initialize topologies and to create and retrieve table objects
 * via names.
 */
class PFabricContext {
public:
  typedef std::shared_ptr<Topology> TopologyPtr;

  /**
   * @brief Creates a new context.
   *
   * Creates a new empty context.
   */
  PFabricContext();

  /**
   * @brief Deletes the context.
   *
   * Deletes the context and frees all resources.
   */
  ~PFabricContext();

  /**
   * @brief Creates a topology.
   *
   * Creates a new empty topology which can be used to construct a new
   * dataflow program.
   *
   * @return
   *    a pointer to a new and empty topology object.
   */
  TopologyPtr createTopology();

  /**
   * @brief Creates a new table with the given name and schema.
   *
   * Creates a new table with the given name. The schema (record type and
   * key) are specified as template parameters. If a table with the same
   * name already exists, then an exception is raised.
   *
   * @tparam RecordType
   *    the record type of the table, usually a @c TuplePtr<Tuple<...> >
   * @tparam KeyType
   *    the data type of the key of the table
   * @param[in] tblName
  *     the name of the table to be created
   * @return
   */
  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> createTable(const std::string& tblName) throw (TableException) {
    // first we check whether the table exists already
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end())
      throw TableException("table already exists");

    // create a new table and regiter it
    auto tbl = std::make_shared<Table<RecordType, KeyType>>();
    mTableSet[tblName] = tbl;
    return tbl;
  }

  /**
   * @brief Gets a table by its name.
   *
   * Retrieves a table with the given schema (record type and key) by
   * its name. If exists then a smart pointer to the table is returned,
   * otherwise an empty pointer is returned.
   *
   * @tparam RecordType
   *    the record type of the table, usually a @c TuplePtr<Tuple<...> >
   * @tparam KeyType
   *    the data type of the key of the table
   * @param[in] tblName
   *    the name of the table
   * @return
   */
  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> getTable(const std::string& tblName) {
    // look for the table
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end()) {
      // if found then we return it
      return std::static_pointer_cast<Table<RecordType, KeyType>>(it->second);
    }
    else
      // otherwise we just return an empty pointer
      return std::shared_ptr<Table<RecordType, KeyType>>();
  }

private:
  typedef std::shared_ptr<BaseTable> BaseTablePtr;

  std::map<std::string, BaseTablePtr> mTableSet; /// a dictionary collecting all existing tables

};

}

#endif
