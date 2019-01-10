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

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include <boost/filesystem.hpp>

#include "core/Tuple.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"

using namespace pfabric;
using namespace ns_types;

TEST_CASE("Building and running a topology with unpartitioned aggregation", 
        "[Unpartitioned Aggregation]") {
  typedef TuplePtr<unsigned long, double> MyTuplePtr;
  typedef TuplePtr<double> AggregationResultPtr;
  typedef Aggregator1<MyTuplePtr, AggrSum<double>, 1> AggrStateSum;
        
  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    return makeTuplePtr(n, (double)n + 0.5); 
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<double> results;

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .aggregate<AggrStateSum>()
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num)
          results.push_back(get<0>(tp));
        tuplesProcessed++;
    });

  t.start(false);

  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == num);

  for (auto i=0u; i<num; i++) {
    if (i==0) REQUIRE(results[i] == 0.5);
    else REQUIRE(results[i] == results[i-1]+i+0.5);
  }
}

TEST_CASE("Building and running a topology with partitioned aggregation", 
        "[Partitioned Aggregation]") {
  typedef TuplePtr<unsigned long, double> MyTuplePtr;
  typedef TuplePtr<double> AggregationResultPtr;
  typedef Aggregator1<MyTuplePtr, AggrSum<double>, 1> AggrStateSum;

  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    return makeTuplePtr(n, (double)n + 0.5); 
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<double> results;

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .partitionBy([](auto tp) { return get<0>(tp) % 5; }, 5)
    .aggregate<AggrStateSum>()
    .merge() //TODO: Use new merge operator
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num)
          results.push_back(get<0>(tp));
        tuplesProcessed++;
    });

  t.start(false);

  std::this_thread::sleep_for(2s);

  /*for (auto i=0u; i<results.size(); i++) {
  	std::cout<<results[i]<<std::endl;
  }*/

  REQUIRE(results.size() == num);
  //REQUIRE(results[num-1] == 500000);
}
