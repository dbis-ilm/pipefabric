/*
 * Copyright (c) 2014-17 The PipeFabric team,
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

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include <boost/core/ignore_unused.hpp>

#include <boost/filesystem.hpp>

#include "TestDataGenerator.hpp"

#include "core/Tuple.hpp"

#include "table/Table.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"

#include "benchmark/include/benchmark/benchmark.h"

using namespace pfabric;
using namespace ns_types;

//Syntax for benchmarking: "method(benchmark::State& state)"
//The state is needed for testing.
//Additionally, the method has to be registered by
//"BENCHMARK(method)".

/**
 *Testing method one: "map" before "where"
 *Here a projection is done before a selection (bad), resulting in higher
 *running time needed for finishing the query.
 */
void TopologyMapWhereTest(benchmark::State& state) {
	
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double, int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  //code inside the while-loop is tested (in this case for running time)
  while (state.KeepRunning()) {	  
    Topology t;
    auto s = t.newStreamFromFile("file.csv")
      .extract<T1>(',')
	  //map <int, string, double> to <double, int>
      .map<T2>([](auto tp, bool outdated) -> T2 {
        return makeTuplePtr(get<2>(tp), get<0>(tp));
      })
	  //remove tuples whose <int> value is no multiple of 50
	  .where([](auto tp, bool outdated) { return get<1>(tp) % 50 == 0; });

    t.start();
    t.wait();
  }
}
//register method for testing
BENCHMARK(TopologyMapWhereTest);

/**
 *Testing method two: "where" before "map"
 *Here a selection is done before a projection (good), resulting in lower
 *running time needed for finishing the query.
 */
void TopologyWhereMapTest(benchmark::State& state) {
	
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double, int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  while (state.KeepRunning()) {
    Topology t;
    auto s = t.newStreamFromFile("file.csv")
      .extract<T1>(',')
	  .where([](auto tp, bool outdated) { return get<0>(tp) % 50 == 0; })
      .map<T2>([](auto tp, bool outdated) -> T2 {
        return makeTuplePtr(get<2>(tp), get<0>(tp));
      });

    t.start();
    t.wait();
  }
}
BENCHMARK(TopologyWhereMapTest);

/**
 *Testing method three: partitioned "where" before "map"
 *In addition to method two, a partitioning with three partitions is used.
 *Because of simple and fast operators, overhead is higher than gain, resulting
 *in higher running time.
 */
void TopologyPartitionedWhereBeforeMapTest(benchmark::State& state) {
	
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double, int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  while (state.KeepRunning()) {
    Topology t;
    auto s = t.newStreamFromFile("file.csv")
      .extract<T1>(',')
	  .partitionBy([](auto tp) { return get<0>(tp) % 3; }, 3)
	  .where([](auto tp, bool outdated) { return get<0>(tp) % 50 == 0; })
      .map<T2>([](auto tp, bool outdated) -> T2 {
        return makeTuplePtr(get<2>(tp), get<0>(tp));
      })
	  .merge();

    t.start();
    t.wait();

	//BAD: Takes far too long because of iteration number
	//wait for results - stop timer
	//state.PauseTiming();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//state.ResumeTiming();
  }
}
BENCHMARK(TopologyPartitionedWhereBeforeMapTest);

//Some math operation used for next two testing methods
double doMath(double input) {
	double result = 0;
	
	for (auto i=0; i<10; i++)
		for (auto j=0; j<10; j++)
			for (auto k=0; k<10; k++)
				result += std::sqrt(123.456L*i*j*k*input);

	return result;
}

/**
 *Testing method four: "groupby"
 *Here the groupby operator is tested along with some math in a mapping
 *operator.
 */
void TopologyGroupByTest(benchmark::State& state) {
	
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double> T2;
  typedef Aggregator1<T1, AggrSum<double>, 2> AggrStateSum;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  while (state.KeepRunning()) {  
    Topology t;
    auto s = t.newStreamFromFile("file.csv")
      .extract<T1>(',')
      .keyBy<int>([](auto tp) { return get<0>(tp); })
      .groupBy<AggrStateSum, int>()
	  .map<T2>([](auto tp, bool outdated) -> T2 {
		double math = doMath(get<0>(tp));
        return makeTuplePtr(math);
      });
	  
    t.start(false);
  }
}
BENCHMARK(TopologyGroupByTest);

/**
 *Testing method five: partitioned "groupby"
 *Here the groupby operator along with some math in a mapping operator is
 *tested with three partitions. Because of more complexity in the two operators,
 *this results in much lower running time compared to no partitioning.
 */
void TopologyPartitionedGroupByTest(benchmark::State& state) {
	
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double> T2;
  typedef Aggregator1<T1, AggrSum<double>, 2> AggrStateSum;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  while (state.KeepRunning()) {
    Topology t;
    auto s = t.newStreamFromFile("file.csv")
      .extract<T1>(',')
	  .keyBy<int>([](auto tp) { return get<0>(tp); })
	  .partitionBy([](auto tp) { return get<0>(tp) % 3; }, 3)
      .groupBy<AggrStateSum, int>()
	  .map<T2>([](auto tp, bool outdated) -> T2 {
        double math = doMath(get<0>(tp));
        return makeTuplePtr(math);
      })
	  .merge(); 

    t.start(false);
	  
	//BAD: Takes far too long because of iteration number
	//wait for results - stop timer
	//state.PauseTiming();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//state.ResumeTiming();
  }
}
BENCHMARK(TopologyPartitionedGroupByTest);

/**
 *Testing method six: partitioned join
 *ERROR while testing: "double free or corruption (out)"
 */
void TopologyPartitionedJoinTest(benchmark::State& state) {
	
  typedef TuplePtr<int, std::string, double> T1;

  TestDataGenerator tgen1("file1.csv");
  tgen1.writeData(100);
  TestDataGenerator tgen2("file2.csv");
  tgen2.writeData(100);

  while (state.KeepRunning()) {
    Topology t;
    auto s1 = t.newStreamFromFile("file1.csv")
      .extract<T1>(',')
	  .keyBy<int>([](auto tp) { return get<0>(tp); });
	  
    auto s2 = t.newStreamFromFile("file2.csv")
      .extract<T1>(',')	  
	  .partitionBy([](auto tp) { return get<0>(tp) % 3; }, 3)
	  .keyBy<int>([](auto tp) { return get<0>(tp); })
	  .join<int>(s1, [](auto tp1, auto tp2) { return true; })
	  .merge();

    t.start();
	  t.wait();  
	//BAD: Takes far too long because of iteration number
	//wait for results - stop timer
	//state.PauseTiming();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//state.ResumeTiming();
  }
}
BENCHMARK(TopologyPartitionedJoinTest);

//MAIN for benchmark tests
BENCHMARK_MAIN();
