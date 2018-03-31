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

#ifndef PFabricContext_hpp_
#define PFabricContext_hpp_

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "core/PFabricTypes.hpp"
#include "dsl/Dataflow.hpp"
#include "qop/Queue.hpp"
#include "table/Table.hpp"
#include "qop/Queue.hpp"
#include "dsl/Topology.hpp"
#ifdef SUPPORT_MATRICES
#include "matrix/Matrix.hpp"
#endif

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
         a pointer to the newly created table
   */
  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> createTable(const std::string& tblName) noexcept(false) {
    // first we check whether the table exists already
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end())
      throw TableException("table already exists");

    // create a new table and register it
    auto tbl = std::make_shared<Table<RecordType, KeyType>>(tblName);
    mTableSet[tblName] = tbl;
    return tbl;
  }

  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> createTable(const TableInfo& tblInfo) noexcept(false) {
    // first we check whether the table exists already
    auto it = mTableSet.find(tblInfo.tableName());
    if (it != mTableSet.end())
      throw TableException("table already exists");

    // create a new table and register it
    auto tbl = std::make_shared<Table<RecordType, KeyType>>(tblInfo);
    mTableSet[tblInfo.tableName()] = tbl;
    return tbl;
  }

  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<TxTable<RecordType, KeyType>> createTxTable(const TableInfo& tblInfo) throw (TableException) {
    // first we check whether the table exists already
    auto it = mTableSet.find(tblInfo.tableName());
    if (it != mTableSet.end())
      throw TableException("table already exists");

    // create a new table and register it
    auto tbl = std::make_shared<TxTable<RecordType, KeyType>>(tblInfo);
    mTableSet[tblInfo.tableName()] = tbl;
    return tbl;
  }

  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<MVCCTable<RecordType,
                            KeyType>> createMVCCTable(const TableInfo &tblInfo,
                                                      StateContext<RecordType,
                                                        KeyType> &sCtx) {
    // first we check whether the table exists already
    auto it = mTableSet.find(tblInfo.tableName());
    if (it != mTableSet.end())
      throw TableException("table already exists");

    // create a new table and register it
    auto tbl = std::make_shared<MVCCTable<RecordType, KeyType>>(tblInfo, sCtx);
    mTableSet[tblInfo.tableName()] = tbl;
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
   *   a pointer to the table with the given or an empty table
   */
  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> getTable(const std::string& tblName) {
    // look for the table
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end()) {
      // if found then we return it
      return std::static_pointer_cast<Table<RecordType, KeyType>>(it->second);
    }
    else {
      // otherwise we just return an empty pointer
      // TODO: shouldn't we throw an exception here???
      std::cout << "table '" << tblName << "' not found" << std::endl;
      return std::shared_ptr<Table<RecordType, KeyType>>();
    }
  }

  TableInfoPtr getTableInfo(const std::string& tblName);

#ifdef SUPPORT_MATRICES
  template<typename T>
  std::shared_ptr<T> createMatrix(const std::string &matrixName) {
    auto it = matrixMap.find(matrixName);
    if(it != matrixMap.end()) {
      throw std::logic_error("matrix already exists");
    }
    auto m = std::make_shared<T>();
    matrixMap[matrixName] = m;
    return m;
  }

  template<typename T>
  std::shared_ptr<T> getMatrix(const std::string &matrixName) {
    auto it = matrixMap.find(matrixName);
    if (it != matrixMap.end()) {
      return std::static_pointer_cast<T>(it->second);
    }
    throw std::logic_error("matrix not found");
  }
#endif

  /**
   * @brief Creates a new stream with the given name and schema.
   *
   * Creates a new stream with the given name. A named stream is only
   * a queue into which tuples can be pushed and where other topologies
   * can be specified to read from.
   * The schema is specified as template parameters. If a stream with the
   * same name already exists, then an exception is raised.
   *
   * @tparam StreamElement
   *    the type of the stream, usually a @c TuplePtr<Tuple<...> >
   * @param[in] streamName
   *     the name of the stream to be created
   * @return
   *     a pointer to the newly created stream
   */
  template <typename StreamElement>
  Dataflow::BaseOpPtr createStream(const std::string& streamName) {
    auto streamOp = std::make_shared<Queue<StreamElement>>();
    // TODO: check whether the stream already exists
    mStreamSet[streamName] = streamOp;
    return streamOp;
  }

private:
  using BaseTablePtr = typename std::shared_ptr<BaseTable>;

  std::map<std::string, BaseTablePtr> mTableSet;         //< a dictionary collecting all existing tables
  std::map<std::string, Dataflow::BaseOpPtr> mStreamSet; //< a dictionary collecting all named streams
#ifdef SUPPORT_MATRICES
  using BaseMatrixPtr = typename std::shared_ptr<BaseMatrix>;
  std::map<std::string, BaseMatrixPtr> matrixMap;        //< a dictionary collecting all existing matrix
#endif
    };

}

#endif
