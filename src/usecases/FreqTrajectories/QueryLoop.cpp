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
