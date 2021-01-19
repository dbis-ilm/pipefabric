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

#include "catch.hpp"

#include "pfabric.hpp"

using namespace pfabric;

/* Use case based on the paper "Linear Road: A Stream Data Management Benchmark" from
 * Arvind Arasu, Mitch Cherniack, Eduardo Galvez, David Maier, Anurag Maskey, Esther Ryvkina,
 * Michael Stonebraker and Richard Tibbetts, in Proceedings of the 30th International Conference
 * on Very Large Data Bases (VLDB), August, 2004. 
 */

/*
 * This test case runs the linear road producer from PipeFabric with the sample file (280 lines).
 * It is important to mention that this test case takes some time because the tuples are delivered
 * according to their timestamps.
 *
 * Hint: If this test is executed directly (./LinearRoadTest), you have to switch into /build/test
 *       folder before.
 */
TEST_CASE("Running the linear road producer with sample file", "[LinearRoad]") {
  typedef TuplePtr<int, int, int, int, int,
                   int, int, int, int, int,
                   int, int, int, int, int> lrTuples; //format, see Linear Road benchmark

  PFabricContext ctx;
  auto t = ctx.createTopology();

  std::vector<int> res;
  auto s = t->newStreamFromLinRoad<lrTuples>(pfabric::gDataPath + "linroad/datafile20seconds.dat")
    .notify([&res](auto tp, bool outdated) {
      int v = get<0>(tp);
      res.push_back(v);
    })
    ;

  t->start(true);
  t->wait();

  REQUIRE(res.size() == 280);
}
