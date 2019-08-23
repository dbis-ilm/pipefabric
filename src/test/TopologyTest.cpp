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

#include <boost/core/ignore_unused.hpp>

#include <boost/filesystem.hpp>

#include "TestDataGenerator.hpp"

#include "core/Tuple.hpp"

#include "table/Table.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"
#include "dsl/PFabricContext.hpp"

using namespace pfabric;
using namespace ns_types;

TEST_CASE("Building and running a simple topology", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double, int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(5);

  std::stringstream strm;
  std::string expected = "0.5,0\n200.5,2\n400.5,4\n";

  Topology t;
  auto s1 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .where([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
    .map<T2>([](auto tp, bool outdated) -> T2 {
      return makeTuplePtr(get<2>(tp), get<0>(tp));
    })
    .assignTimestamps([](auto tp) { return Timestamp(get<1>(tp)); })
    .print(strm);

  t.start();
  t.wait();
  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with ZMQ", "[Topology]") {
  typedef TuplePtr<int, int> T1;

  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("tcp://*:5678");

  std::stringstream strm;

  Topology t;
  auto s = t.newAsciiStreamFromZMQ("tcp://localhost:5678")
    .extract<T1>(',')
    .print(strm);

  t.start(false);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);

  auto handle = std::async(std::launch::async, [&publisher](){
    std::vector<std::string> input = {
      "0,10", "1,11", "2,12", "3,13", "4,14", "5,15"
    };
    for(const std::string &s : input) {
      zmq::message_t request (4);
      memcpy (request.data (), s.c_str(), 4);
      publisher.send(request, zmq::send_flags::none);
    }
  });

  handle.get();
  std::this_thread::sleep_for(2s);

  std::string expected = "0,10\n1,11\n2,12\n3,13\n4,14\n5,15\n";

  REQUIRE(strm.str() == expected);

}

TEST_CASE("Building and running a topology with ToTable", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;

  auto testTable = std::make_shared<Table<T1::element_type, int>>("TopTable");

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .keyBy<int>([](auto tp) { return get<0>(tp); })
    .toTable<int>(testTable);

  t.start(false);

  REQUIRE(testTable->size() == 10);

  for (int i = 0; i < 10; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == "This is a string field");
    REQUIRE(get<2>(tp) == i * 100 + 0.5);
  }
	testTable->drop();
}

TEST_CASE("Building and running a topology with partitioning", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  std::vector<int> results;
  std::mutex r_mutex;

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .partitionBy([](auto tp) { return get<0>(tp) % 5; }, 5)
    .where([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
    .map<T2>([](auto tp, bool outdated) -> T2 { return makeTuplePtr(get<0>(tp)); } )
    .merge()
    .notify([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(r_mutex);
      int v = get<0>(tp);
      results.push_back(v);
    });

  t.start();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == 500);

  std::sort(results.begin(), results.end());
  for (auto i = 0u; i < results.size(); i++) {
    REQUIRE(results[i] == i * 2);
  }
}

TEST_CASE("Building and running a topology with batcher", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  int procBatchCount = 0;

  std::vector<int> results;

  //run batch & unbatch singlethreaded
  Topology t1;
  auto s1 = t1.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .batch(10)
    .notify([&](auto tp, bool outdated) {
      procBatchCount++;
    })
    .unbatch<T1>()
    .notify([&](auto tp, bool outdated) {
      results.push_back(get<0>(tp));
    })
    ;

  t1.start(false);

  REQUIRE(procBatchCount == 100);
  REQUIRE(results.size() == 1000);

  for (size_t i = 0; i < results.size(); ++i) {
    REQUIRE(results[i] == i);
  }

  //run batch & unbatch multithreaded with partitioning
  std::mutex mtx;
  procBatchCount = 0;
  int procTupleCount = 0;

  Topology t2;

  auto s2 = t2.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .partitionBy([](auto tp) { return get<0>(tp) % 2; }, 2)
    .batch(10)
    .notify([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(mtx);
      procBatchCount++;
    })
    .unbatch<T1>()
    .merge()
    .notify([&](auto tp, bool outdated) {
      procTupleCount++;
    })
    ;

  t2.start(false);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(procBatchCount == 100);
  REQUIRE(procTupleCount == 1000);
}

TEST_CASE("Building and running a topology with stream generator", "[StreamGenerator]") {
    typedef TuplePtr<int,int,int> MyTuplePtr;

    auto testTable = std::make_shared<Table<MyTuplePtr::element_type, int>>("StreamGenTable");

    StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
        return makeTuplePtr((int)n, (int)n + 10, (int)n + 100); });
    unsigned long num = 1000;

    Topology t;
    auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
            .keyBy<int>([](auto tp) { return get<0>(tp); })
            .toTable<int>(testTable);

    t.start(false);

    REQUIRE(testTable->size() == num);

    for (unsigned int i = 0; i < num; i++) {
      auto tp = testTable->getByKey(i);
      REQUIRE(get<0>(tp) == i);
      REQUIRE(get<1>(tp) == i+10);
      REQUIRE(get<2>(tp) == i+100);
    }

    testTable->drop();
}

TEST_CASE("Building and running a topology with a memory source", "[MemorySource]") {
  typedef TuplePtr<int, std::string, double> T1;
  std::vector<T1> results;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  Topology t;
  auto s = t.newStreamFromMemory<T1>("file.csv")
    .notify([&](auto tp, bool outdated) {
        results.push_back(tp);
    });

  t.prepare();
  t.start(false);

  REQUIRE(results.size() == 10);
}

TEST_CASE("Building and running a topology with grouping", "[GroupBy]") {
    typedef TuplePtr<int, double> T1;
    typedef TuplePtr<double> T2;
    typedef Aggregator1<T1, AggrSum<double>, 1> AggrStateSum;

    StreamGenerator<T1>::Generator streamGen ([](unsigned long n) -> T1 {
		if (n<5) return makeTuplePtr(0, (double)n + 0.5);
	    else return makeTuplePtr((int)n, (double)n + 0.5); });
    unsigned long num = 10;

	std::stringstream strm;
	std::string expected = "0.5\n2\n4.5\n8\n12.5\n5.5\n6.5\n7.5\n8.5\n9.5\n";

    Topology t;
    auto s = t.streamFromGenerator<T1>(streamGen, num)
        .keyBy<int>([](auto tp) { return get<0>(tp); })
        .groupBy<AggrStateSum, int>()
	.print(strm);
	t.start(false);

	REQUIRE(strm.str() == expected);
}

struct MySumState {
  MySumState() : sum(0) {}
  double sum;
};

TEST_CASE("Building and running a topology with stateful map", "[StatefulMap]") {
  typedef TuplePtr<unsigned long, double> MyTuplePtr;
  typedef TuplePtr<double> AggregationResultPtr;
  typedef StatefulMap<MyTuplePtr, AggregationResultPtr, MySumState> TestMap;

  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    return makeTuplePtr(n, (double)n + 0.5);
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<double> results;

  auto mapFun = [&]( const MyTuplePtr& tp, bool, TestMap& self) -> AggregationResultPtr {
                    self.state()->sum += get<1>(tp);
                    return makeTuplePtr(self.state()->sum);
                };

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .statefulMap<AggregationResultPtr, MySumState>(mapFun)
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num)
          results.push_back(get<0>(tp));
        tuplesProcessed++;
    });

  t.start(false);

  REQUIRE(results.size() == num);
  for (auto i=0u; i<num; i++) {
    if (i==0) REQUIRE(results[i] == 0.5);
    else REQUIRE(results[i] == results[i-1]+i+0.5);
  }
}

TEST_CASE("Combining tuples from two streams to one stream", "[ToStream]") {
  typedef TuplePtr<int, std::string, double> T1;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(100);

  int results = 0;
  PFabricContext ctx;
  Dataflow::BaseOpPtr stream = ctx.createStream<T1>("stream");

  Topology t;
  auto s1 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
	.toStream(stream);

  auto s2 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
	.toStream(stream);

  auto s3 = t.fromStream<T1>(stream)
    .notify([&](auto tp, bool outdated) {
      results++;
    });

  t.start();
  t.wait();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(results == 200);
}

TEST_CASE("Tuplifying a stream of RDF strings", "[Tuplifier]") {
  typedef TuplePtr<std::string, std::string, std::string> Triple;
  typedef TuplePtr<std::string, std::string, std::string, std::string> RDFTuple;
  std::vector<RDFTuple> results;
  std::mutex r_mutex;

  Topology t;
  auto s = t.newStreamFromFile(std::string(TEST_DATA_DIRECTORY) + "tuplifier_test1.in")
    .extract<Triple>(',')
    .tuplify<RDFTuple>({ "http://data.org/name", "http://data.org/price", "http://data.org/someOther" },
        TuplifierParams::ORDERED)
    .notify([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(r_mutex);
      results.push_back(tp);
    });
  t.start(false);
  REQUIRE(results.size() == 3);
}

TEST_CASE("Using a window with and without additional function", "[Window]") {
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<int> T2;
  typedef Aggregator1<T1, AggrSum<double>, 2> AggrStateSum;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  std::stringstream strm;
  std::string expected = "0.5\n101\n301.5\n601.5\n901.5\n1201.5\n1501.5\n1801.5\n2101.5\n2401.5\n";

  Topology t1;
  auto s1 = t1.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .slidingWindow(WindowParams::RowWindow, 3)
    .aggregate<AggrStateSum>()
    .print(strm);

  t1.start(false);
  REQUIRE(strm.str() == expected);

  std::stringstream strm2;
  expected = "1.5\n103\n304.5\n604.5\n904.5\n1204.5\n1504.5\n1804.5\n2104.5\n2404.5\n";

  //just increment incoming tuples double-attribute by one
  auto winFunc = [](auto beg, auto end, auto tp) { get<2>(tp)++; return tp; };

  Topology t2;
  auto s2 = t2.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .slidingWindow(WindowParams::RowWindow, 3, winFunc)
    .aggregate<AggrStateSum>()
    .print(strm2);

  t2.start(false);
  REQUIRE(strm2.str() == expected);

  std::stringstream strm3;
  expected = "0\n1\n1\n2\n2\n3\n4\n5\n6\n7\n";

  //find median of ints
  auto winFuncMedian = [](auto beg, auto end, auto tp) {
    std::vector<int> winInts(0);
    for(auto it=beg; it!=end; ++it) {
      winInts.push_back(get<0>(*it));
    }
    std::sort(winInts.begin(), winInts.end());
    return makeTuplePtr(winInts[winInts.size()/2]);
  };

  Topology t3;
  auto s3 = t3.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .map<T2>([](auto tp, bool outdated) -> T2 { return makeTuplePtr(get<0>(tp)); })
    .slidingWindow(WindowParams::RowWindow, 5, winFuncMedian)
    .print(strm3);

  t3.start(false);
  REQUIRE(strm3.str() == expected);
}

TEST_CASE("Building and running a topology with Transactions", "[Transactions]") {
  //transaction id, user id, test string, test double
  typedef TuplePtr<int, int, std::string, double> T1;
  PFabricContext ctx;

  //create table
  TableInfo tblInfo("TestTable", {}, ColumnInfo::UInt_Type);
  auto testTable = ctx.createTxTable<T1::element_type, int>(tblInfo);

  //tuple production
  StreamGenerator<T1>::Generator streamGen ([](unsigned long n) -> T1 {
    return makeTuplePtr((int)n%3, (int)n, (std::string)"test string", (double)n + 0.5);
  });
  unsigned long num = 10;

  //autocommit, else we need to update a state (tracking transactions)
  bool autocommit = true;

  auto t1 = ctx.createTopology();
  //write to table
  auto s1 = t1->streamFromGenerator<T1>(streamGen, num)
    .assignTransactionID([](auto tp) { return get<0>(tp); })
    .keyBy<1, int>()
    .toTxTable<TxTable<T1::element_type, int>>(testTable, autocommit);

  t1->start();
  t1->wait();

  REQUIRE(testTable->size() == 10);

  int tpCnt = 0;

  auto t2 = ctx.createTopology();
  //read from table
  auto s2 = t2->selectFromTxTable<T1, int>(testTable)
    .notify([&](auto tp, bool outdated) {
      tpCnt++;
    });

  t2->start();
  t2->wait();

  REQUIRE(tpCnt == 10);

	testTable->drop();
}
