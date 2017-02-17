#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include <boost/filesystem.hpp>

#include "core/Tuple.hpp"

#include "TestDataGenerator.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"

using namespace pfabric;
using namespace ns_types;

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
    .keyBy<int>([](auto tp) { return getAttribute<0>(tp); });

  auto s2 = t.newStreamFromFile("file1.csv")
    .extract<T1>(',')
    .keyBy<int>([](auto tp) { return getAttribute<0>(tp); })
    .join<int>(s1, [](auto tp1, auto tp2) { return true; })
    .print(strm);

  t.start(false);

  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with a join on one partitioned stream", "[Topology]") {
    typedef TuplePtr<Tuple<unsigned long, unsigned long>> MyTuplePtr;

    StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
        return makeTuplePtr(n, n % 100); 
    });
    unsigned long num = 1000;

    Topology t;
    auto s2 = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
            .keyBy<0>();

    auto s1 = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
            .keyBy<0>()
            .partitionBy([](auto tp) { return get<1>(tp) % 5; }, 5)
            .join(s2, [](auto tp1, auto tp2) { return get<1>(tp1) == get<1>(tp2); })
            .merge()
            .print();

    t.start(false);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

    // REQUIRE(false);
}

TEST_CASE("Building and running a topology with a join on two partitioned streams", "[Topology]") {
}
