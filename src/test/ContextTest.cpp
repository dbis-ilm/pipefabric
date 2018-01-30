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

#include <chrono>
#include <future>
#include <sstream>
#include <thread>

#include "TestDataGenerator.hpp"
#include "core/StreamElementTraits.hpp"
#include "core/Tuple.hpp"
#include "table/Table.hpp"
#include "dsl/PFabricContext.hpp"
#include "dsl/Topology.hpp"

using namespace pfabric;

TEST_CASE("Building and running a topology via the context", "[Context]") {
  typedef TuplePtr<int, std::string, double> T1;

  PFabricContext ctx;

  REQUIRE(!ctx.getTable<T1::element_type>("AnotherTable"));

  auto testTable = ctx.createTable<T1::element_type, int>("MyTable");

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  {
    auto t = ctx.createTopology();

    auto s = t->newStreamFromFile("file.csv")
                 .extract<T1>(',')
                 .keyBy<int>([](auto tp) { return getAttribute<0>(tp); })
                 .toTable<int>(testTable);

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
  typedef TuplePtr<int, std::string, double> T1;

  PFabricContext ctx;

  REQUIRE(!ctx.getTable<T1::element_type>("MyTable"));

  auto testTable = ctx.createTable<T1::element_type, int>("MyTable");

  TestDataGenerator tgen("file.csv");
  tgen.writeData(100);

  {
    auto t = ctx.createTopology();

    auto s = t->newStreamFromFile("file.csv")
                 .extract<T1>(',')
                 .keyBy<int>([](auto tp) { return getAttribute<0>(tp); })
                 .toTable<int>(testTable);

    t->start(false);
    REQUIRE(testTable->size() == 100);
  }

  {
    unsigned int num = 0;
    auto t = ctx.createTopology();

    auto s = t->selectFromTable<T1>(testTable).notify(
        [&](auto tp, bool outdated) { num++; });

    t->start(false);
    REQUIRE(testTable->size() == num);
  }

  testTable->drop();
}
