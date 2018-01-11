/*
 * Copyright (c) 2014-18 The PipeFabric team,
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


TEST_CASE("Creating a table with a given schema and deleting data",
          "[NVMTable]") {
  auto testTable = std::make_shared<TableType>("MyTestTable2");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple((unsigned long)i, i + 100, fmt::format("String#{}", i),
                      i / 100.0);
    testTable->insert(i, tp);
  }

  REQUIRE(testTable->size() == 10000);
  for (int i = 0; i < 10000; i += 100) testTable->deleteByKey(i);

  //TODO: REQUIRE(testTable->size() == 9900);
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

TEST_CASE("scanning the whole table", "[NVMTable]") {
  auto testTable = std::make_shared<TableType>("MyTestTable7");
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

TEST_CASE("scanning the table with a predicate", "[NVMTable]") {
  auto testTable = std::make_shared<TableType>("MyTestTable8");
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