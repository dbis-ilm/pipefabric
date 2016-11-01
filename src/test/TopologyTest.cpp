#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

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

#include "topology/Topology.hpp"
#include "topology/Pipe.hpp"

using namespace pfabric;
using namespace ns_types;

TEST_CASE("Building and running a simple topology", "[Topology]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;
  typedef TuplePtr<Tuple<double, int> > T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(5);

  std::stringstream strm;
  std::string expected = "0.5,0\n200.5,2\n400.5,4\n";

  Topology t;
  auto s1 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .where<T1>([](auto tp, bool outdated) { return getAttribute<0>(tp) % 2 == 0; } )
    .map<T1,T2>([](auto tp, bool outdated) -> T2 {
      return makeTuplePtr(getAttribute<2>(tp), getAttribute<0>(tp));
    })
    .assignTimestamps<T2>([](auto tp) { return getAttribute<1>(tp); })
    .print<T2>(strm);

  t.start();
  t.wait();
  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with joins", "[Topology]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;
  typedef TuplePtr<Tuple<int, std::string, double, int, std::string, double> > T2;

  TestDataGenerator tgen1("file1.csv");
  tgen1.writeData(5);

  TestDataGenerator tgen2("file2.csv");
  tgen2.writeData(8);

  std::stringstream strm;
  std::string expected = "0,This is a string field,0.5,0,This is a string field,0.5\n\
1,This is a string field,100.5,1,This is a string field,100.5\n\
2,This is a string field,200.5,2,This is a string field,200.5\n\
3,This is a string field,300.5,3,This is a string field,300.5\n\
4,This is a string field,400.5,4,This is a string field,400.5\n";

  Topology t;
  auto s1 = t.newStreamFromFile("file2.csv")
    .extract<T1>(',')
    .keyBy<T1, int>([](auto tp) { return getAttribute<0>(tp); });

  auto s2 = t.newStreamFromFile("file1.csv")
    .extract<T1>(',')
    .keyBy<T1, int>([](auto tp) { return getAttribute<0>(tp); })
    .join<T1, T1, int>(s1, [](auto tp1, auto tp2) { return true; })
    .print<T2>(strm);

  t.start(false);

  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with ZMQ", "[Topology]") {
  typedef TuplePtr<Tuple<int, int> > T1;

  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("tcp://*:5678");

  std::stringstream strm;

  Topology t;
  auto s = t.newStreamFromZMQ("tcp://localhost:5678")
    .extract<T1>(',')
    .print<T1>(strm);

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
      publisher.send (request);
    }
  });

  handle.get();
  std::this_thread::sleep_for(2s);

  std::string expected = "0,10\n1,11\n2,12\n3,13\n4,14\n5,15\n";

  REQUIRE(strm.str() == expected);

}

TEST_CASE("Building and running a topology with ToTable", "[Topology]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;

  auto testTable = std::make_shared<Table<T1, int>>();

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .keyBy<T1, int>([](auto tp) { return getAttribute<0>(tp); })
    .toTable<T1, int>(testTable);

  t.start(false);

  REQUIRE(testTable->size() == 10);

  for (int i = 0; i < 10; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(tp->getAttribute<0>() == i);
    REQUIRE(tp->getAttribute<1>() == "This is a string field");
    REQUIRE(tp->getAttribute<2>() == i * 100 + 0.5);
  }
}

TEST_CASE("Building and running a topology with partitioning", "[Topology]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;
  typedef TuplePtr<Tuple<int> > T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  std::vector<int> results;
  std::mutex r_mutex;

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .partitionBy<T1>([](auto tp) { return getAttribute<0>(tp) % 5; }, 5)
    .where<T1>([](auto tp, bool outdated) { return getAttribute<0>(tp) % 2 == 0; } )
    .map<T1,T2>([](auto tp, bool outdated) -> T2 { return makeTuplePtr(getAttribute<0>(tp)); } )
    .merge<T2>()
    .notify<T2>([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(r_mutex);
      int v = getAttribute<0>(tp);
      results.push_back(v);
    });

  std::cout << "start topology" << std::endl;
  t.start(false);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == 500);

  std::sort(results.begin(), results.end());
  for (int i = 0; i < results.size(); i++) {
    REQUIRE(results[i] == i * 2);
  }
}

