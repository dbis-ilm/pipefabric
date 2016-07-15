#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include "core/Tuple.hpp"
#include "table/Table.hpp"
#include "topology/PFabricContext.hpp"
#include "TestDataGenerator.hpp"

using namespace pfabric;

TEST_CASE("Building and running a topology via the context", "[Context]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;

  PFabricContext ctx;

  REQUIRE(! ctx.getTable<T1>("AnotherTable"));

  auto testTable = ctx.createTable<T1, int>("MyTable");

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

  auto tbl = ctx.getTable<T1, int>("MyTable");
  REQUIRE(tbl->size() == 10);

  for (int i = 0; i < 10; i++) {
    auto tp = tbl->getByKey(i);
    REQUIRE(tp->getAttribute<0>() == i);
    REQUIRE(tp->getAttribute<1>() == "This is a string field");
    REQUIRE(tp->getAttribute<2>() == i * 100 + 0.5);
  }
}
