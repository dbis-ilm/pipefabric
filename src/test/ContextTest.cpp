#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file

#include "catch.hpp"

#include <chrono>
#include <future>
#include <sstream>
#include <thread>

#include "TestDataGenerator.hpp"
#include "core/Tuple.hpp"
#include "table/Table.hpp"
#include "topology/PFabricContext.hpp"

using namespace pfabric;

TEST_CASE("Building and running a topology via the context", "[Context]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;

  PFabricContext ctx;

  REQUIRE(!ctx.getTable<T1::element_type>("AnotherTable"));

  auto testTable = ctx.createTable<T1::element_type, int>("MyTable");

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  {
    auto t = ctx.createTopology();

    auto s = t->newStreamFromFile("file.csv")
                 .extract<T1>(',')
                 .keyBy<T1, int>([](auto tp) { return getAttribute<0>(tp); })
                 .toTable<T1, int>(testTable);

    t->start(false);
  }

  auto tbl = ctx.getTable<T1::element_type, int>("MyTable");
  REQUIRE(tbl->size() == 10);

  for (int i = 0; i < 10; i++) {
    auto tp = tbl->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == "This is a string field");
    REQUIRE(get<2>(tp) == i * 100 + 0.5);
  }
  testTable->drop();
}

TEST_CASE("Building and running a topology with SelectFromTable",
          "[Context][Topology]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;

  PFabricContext ctx;

  REQUIRE(!ctx.getTable<T1::element_type>("MyTable"));

  auto testTable = ctx.createTable<T1::element_type, int>("MyTable");

  TestDataGenerator tgen("file.csv");
  tgen.writeData(100);

  {
    auto t = ctx.createTopology();

    auto s = t->newStreamFromFile("file.csv")
                 .extract<T1>(',')
                 .keyBy<T1, int>([](auto tp) { return getAttribute<0>(tp); })
                 .toTable<T1, int>(testTable);

    t->start(false);
    REQUIRE(testTable->size() == 100);
  }

  {
    unsigned int num = 0;
    auto t = ctx.createTopology();

    auto s = t->selectFromTable<T1>(testTable).notify<T1>(
        [&](auto tp, bool outdated) { num++; });

    t->start(false);
    REQUIRE(testTable->size() == num);
  }

  testTable->drop();
}
