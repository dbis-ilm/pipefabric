#include <iostream>
#include <cstdlib>

#include <boost/program_options.hpp>

#include "pfabric.hpp"
#include "TopologyBuilder.hpp"
#include "SQLParser.hpp"
#include "Plan.hpp"
#include "QueryCompiler.hpp"

using namespace pfabric;

namespace po = boost::program_options;

typedef TuplePtr<Tuple<int, double> > InTuplePtr;

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
