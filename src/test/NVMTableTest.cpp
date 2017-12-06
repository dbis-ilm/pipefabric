#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do
// this in one cpp file


#include "catch.hpp"
#include "fmt/format.h"

#include "core/Tuple.hpp"
#include "pfabric.hpp"
#include "table/NVMTable.hpp"

using namespace pfabric;

using MyTuple = Tuple<int, int, string, double>;
using TableType = NVMTable<MyTuple, int>;

TEST_CASE("Creating a table with a given schema and inserting data",
          "[NVMTable]") {
  auto testTable = std::make_shared<TableType>("MyTestTable1");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long) i, i + 100, fmt::format("String#{}", i), i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  for (unsigned long i = 0; i < 10000; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == (int) (i + 100));
    REQUIRE(get<2>(tp) == fmt::format("String#{}", i));
    REQUIRE(get<3>(tp) == i / 100.0);
  }
  testTable->drop();
}
