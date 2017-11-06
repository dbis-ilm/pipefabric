#include "pfabric.hpp"
#include "qcomp/TopologyBuilder.hpp"
#include "qcomp/SQLParser.hpp"
#include "qcomp/Plan.hpp"
#include "qcomp/QueryCompiler.hpp"

#include "TrajectoryDB.hpp"

using namespace pfabric;

void queryLoop(PFabricContext& ctx) {
  boost::filesystem::path library_path(".");
  std::list<TopologyBuilderPtr> activeTopologies;
  QueryCompiler sqlCompiler;
  sqlCompiler.readSettings(library_path);

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
