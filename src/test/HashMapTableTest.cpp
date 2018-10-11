/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#include <vector>

#include "pfabric.hpp"
#include "core/Tuple.hpp"
#include "table/HashMapTable.hpp"
#include "fmt/format.h"

using namespace pfabric;

typedef pfabric::Tuple<unsigned long, int, std::string, double> MyTuple;

template <typename RecordType, typename KeyType = DefaultKeyType>
using HTable = HashMapTable<RecordType, KeyType>;

TEST_CASE("Creating a table with a given schema, inserting and deleting data", "[Table]") {
  auto testTable = std::make_shared<HTable<MyTuple>> ();
  for (int i = 0; i < 10000; i++) {
    auto tp =MyTuple((unsigned long) i, i + 100, fmt::format("String#{}", i), i / 100.0);
    testTable->insert(i, tp);
  }

  SECTION("checking inserts of data") {
    REQUIRE(testTable->size() == 10000);
    for (int i = 0; i < 10000; i++) {
      auto tp = testTable->getByKey(i);
      REQUIRE(get<0>(tp) == i);
      REQUIRE(get<1>(tp) == i + 100);
      REQUIRE(get<2>(tp) == fmt::format("String#{}", i));
      REQUIRE(get<3>(tp) == i / 100.0);
    }
  }

  SECTION("deleting data by key") {
    REQUIRE(testTable->size() == 10000);
    for (int i = 0; i < 10000; i += 100)
      testTable->deleteByKey(i);

    REQUIRE(testTable->size() == 9900);
    // check if the records were really deleted
    for (int i = 0; i < 10000; i += 100) {
      try {
        auto tp = testTable->getByKey(i);
        REQUIRE(false);
      }
      catch (TableException& exc) {
        // if the key wasn't found then an exception is raised
        // which we can ignore here
      }
    }
  }

  SECTION("deleting data using a predicate") {
    REQUIRE(testTable->size() == 10000);
    auto num = testTable->deleteWhere([](const MyTuple& tp) -> bool {
      return get<0>(tp) % 100 == 0;
    });
    REQUIRE(num == 100);
    REQUIRE(testTable->size() == 9900);
    for (int i = 0; i < 10000; i += 100) {
      try {
        auto tp = testTable->getByKey(i);
        REQUIRE(false);
      }
      catch (TableException& exc) {
        // if the key wasn't found then an exception is raised
        // which we can ignore here
      }
    }
  }

  SECTION("updating some data by key") {
    REQUIRE(testTable->size() == 10000);
    for (int i = 100; i < 10000; i += 100) {
      testTable->updateByKey(i, [](MyTuple& tp) {
          get<1>(tp) += 100;
      });
    }
    for (int i = 100; i < 10000; i += 100) {
      auto tp = testTable->getByKey(i);
      REQUIRE(get<1>(tp) == get<0>(tp) + 200);
    }
  }

  SECTION("updating some data by predicate") {
    REQUIRE(testTable->size() == 10000);
    testTable->updateWhere(
      [](const MyTuple& tp) -> bool {
        return get<0>(tp) % 100 == 0;
      },
      [](MyTuple& tp) {
        get<1>(tp) += 100;
    });
    for (int i = 0; i < 10000; i += 100) {
      auto tp = testTable->getByKey(i);
      REQUIRE(get<1>(tp) == get<0>(tp) + 200);
    }
  }

  SECTION("observing inserts, deletes, and updates on a table") {
    REQUIRE(testTable->size() == 10000);
    bool insertDetected = false,
        deleteDetected = false,
        updateDetected = false;

    auto observer = [&insertDetected, &deleteDetected, &updateDetected](const MyTuple& rec,
      TableParams::ModificationMode mode) {
      switch (mode) {
      case TableParams::Insert:
        if (get<0>(rec) == 20000lu)
          insertDetected = true;
        break;
      case TableParams::Delete:
        if (get<0>(rec) == 20000lu)
          deleteDetected = true;
        break;
      case TableParams::Update:
        if (get<0>(rec) == 5000lu)
          updateDetected = true;
        break;
      default:
        break;
      }
    };
    testTable->registerObserver(observer, TableParams::Immediate);
    testTable->insert(20000, MyTuple(20000lu, 20, std::string("A String"), 100.0));
    REQUIRE(insertDetected == true);

    testTable->deleteByKey(20000);
    REQUIRE(deleteDetected == true);

    testTable->updateByKey(5000, [](MyTuple& tp) {
        get<1>(tp) += 100;
    });
    REQUIRE(updateDetected == true);
  }

  SECTION("scanning the whole table") {
    REQUIRE(testTable->size() == 10000);

    unsigned int num = 0;
    auto iter = testTable->select();
    for (; iter.isValid(); iter++)
      num++;

    REQUIRE(num == testTable->size());
  }

  SECTION("scanning the table with a predicate") {
    REQUIRE(testTable->size() == 10000);

    unsigned int num = 0;
    auto iter = testTable->select([](const MyTuple& tp) {
      return get<0>(tp) % 2 == 0;
    });
    for (; iter.isValid(); iter++) {
      REQUIRE(get<0>(*iter) % 2 == 0);
      num++;
    }
    REQUIRE(num == testTable->size() / 2);
  }
}
