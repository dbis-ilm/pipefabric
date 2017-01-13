#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file

#include "catch.hpp"

#include <vector>

#include "pfabric.hpp"

#include "table/LDBTable.hpp"

#include "fmt/format.h"

using namespace pfabric;

typedef Tuple<unsigned long, int, std::string, double> MyTuple;

template <typename RecordType, typename KeyType>
using LTable = LDBTable<RecordType, KeyType>;

TEST_CASE("Creating a table with a given schema and inserting data",
          "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable1");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  for (unsigned long i = 0; i < 10000; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == (int)(i + 100));
    REQUIRE(get<2>(tp) == fmt::format("String#{}", i));
    REQUIRE(get<3>(tp) == i / 100.0);
  }
  testTable->drop();
}

TEST_CASE("Creating a table with a given schema and deleting data",
          "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable2");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  for (int i = 0; i < 10000; i += 100) testTable->deleteByKey(i);

  REQUIRE(testTable->size() == 9900);
  // check if the records were really deleted
  for (int i = 0; i < 10000; i += 100) {
    try {
      auto tp = testTable->getByKey(i);
      REQUIRE(false);
    } catch (TableException& exc) {
      // if the key wasn't found then an exception is raised
      // which we can ignore here
    }
  }
  testTable->drop();
}

TEST_CASE(
    "Creating a table with a given schema and deleting data using a predicate",
    "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable3");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  auto num = testTable->deleteWhere(
      [](const MyTuple& tp) -> bool { return get<0>(tp) % 100 == 0; });
  REQUIRE(num == 100);
  REQUIRE(testTable->size() == 9900);
  for (int i = 0; i < 10000; i += 100) {
    try {
      auto tp = testTable->getByKey(i);
      REQUIRE(false);
    } catch (TableException& exc) {
      // if the key wasn't found then an exception is raised
      // which we can ignore here
    }
  }
  testTable->drop();
}

TEST_CASE("updating some data by key in a table", "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable4");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  for (int i = 100; i < 10000; i += 100) {
    testTable->updateByKey(i, [](MyTuple& tp) { get<1>(tp) += 100; });
  }
  for (int i = 100; i < 10000; i += 100) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<1>(tp) == get<0>(tp) + 200);
  }
  testTable->drop();
}

TEST_CASE("updating some data by predicate in a table", "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable5");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  testTable->updateWhere(
      [](const MyTuple& tp) -> bool { return get<0>(tp) % 100 == 0; },
      [](MyTuple& tp) { get<1>(tp) += 100; });
  for (int i = 0; i < 10000; i += 100) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<1>(tp) == get<0>(tp) + 200);
  }
  testTable->drop();
}

TEST_CASE("observing inserts, deletes, and updates on a table", "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable6");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  bool insertDetected = false, deleteDetected = false, updateDetected = false;

  auto observer = [&insertDetected, &deleteDetected, &updateDetected](
      const MyTuple& rec, TableParams::ModificationMode mode) {
    switch (mode) {
      case TableParams::Insert:
        if (get<0>(rec) == 20000lu) insertDetected = true;
        break;
      case TableParams::Delete:
        if (get<0>(rec) == 20000lu) deleteDetected = true;
        break;
      case TableParams::Update:
        if (get<0>(rec) == 5000lu) updateDetected = true;
        break;
      default:
        break;
    }
  };
  testTable->registerObserver(observer, TableParams::Immediate);
  testTable->insert(20000,
                    MyTuple(20000lu, 20, std::string("A String"), 100.0));
  REQUIRE(insertDetected == true);

  testTable->deleteByKey(20000);
  REQUIRE(deleteDetected == true);

  testTable->updateByKey(5000, [](MyTuple& tp) { get<1>(tp) += 100; });
  REQUIRE(updateDetected == true);
  testTable->drop();
}

TEST_CASE("scanning the whole table", "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable7");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);

  unsigned int num = 0;
  for (auto iter = testTable->select(); iter.isValid(); ++iter) num++;

  REQUIRE(num == testTable->size());
  testTable->drop();
}

TEST_CASE("scanning the table with a predicate", "[LDBTable]") {
  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable8");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);

  unsigned int num = 0;
  {
    auto iter = testTable->select(
        [](const MyTuple& tp) { return get<0>(tp) % 2 == 0; });
    for (; iter.isValid(); iter++) {
      REQUIRE(get<0>(*iter) % 2 == 0);
      num++;
    }
    REQUIRE(num == testTable->size() / 2);
  }
  testTable->drop();
}
