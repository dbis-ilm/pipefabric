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

#include <iostream>
#include <cstdlib>

#include <boost/program_options.hpp>

#include "pfabric.hpp"
#include "TopologyBuilder.hpp"
#include "SQLParser.hpp"
#include "Plan.hpp"
#include "QueryCompiler.hpp"
#include "table/TableInfo.hpp"


using namespace pfabric;

namespace po = boost::program_options;

typedef TuplePtr<int, double> InTuplePtr;

PFabricContext::TopologyPtr createStreamQuery(PFabricContext& ctx) {
  auto myTable = ctx.getTable<InTuplePtr::element_type, int>("SENSOR_DATA");

  auto topology = ctx.createTopology();

  auto s = topology->newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
    .extractJson<InTuplePtr>({"key", "data"})
    .keyBy<0, int>()
    // .print<InTuplePtr>();
    .toTable<int>(myTable);

  return topology;
}

int main(int argc, char **argv) {
  PFabricContext ctx;
  std::list<TopologyBuilderPtr> activeTopologies;

  boost::filesystem::path library_path(argc > 1 ? argv[1] : ".");

  QueryCompiler sqlCompiler;
  sqlCompiler.readSettings(library_path);

  TableInfo tInfo("SENSOR_DATA", 
      { ColumnInfo("col1", ColumnInfo::Int_Type), 
        ColumnInfo("col2", ColumnInfo::Double_Type) }, 
      ColumnInfo::Int_Type);

  ctx.createTable<InTuplePtr::element_type, int>(tInfo);

  auto sQuery = createStreamQuery(ctx);
  sQuery->start(true);

  // sqlCompiler.execQuery(ctx, "select col1 from SENSOR_DATA where col1 = 42 and col2 > col1");

  bool finished = false;
  do {
    std::string buf;
    std::cout << "pfabric> " << std::flush;
    auto& s = std::getline (std::cin, buf);
    if (!s) {
      finished = true;
    }
    else {
      if (buf.length() > 0) {
        try {
          auto topology = sqlCompiler.execQuery(ctx, buf);
          activeTopologies.push_back(topology);
        } catch (exception& exc) {
          std::cout << exc.what() << std::endl;
        }
      }
    }
  } while (!finished);

}
