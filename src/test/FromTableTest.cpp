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

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/ToTable.hpp"
#include "qop/FromTable.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, int, int > MyTuplePtr;

/**
 * A simple test of the FromTable operator.
 */
TEST_CASE("Producing a data stream from inserts into a table", "[FromTable]") {
  typedef Table<MyTuplePtr::element_type, int> MyTable;
  auto testTable = std::make_shared<MyTable>("MyTable");

  for (int i = 0; i < 10; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    testTable->insert(i, *tp);
  }

  auto op = std::make_shared<FromTable<MyTuplePtr, int> >(testTable);

	std::vector<MyTuplePtr> expected;

  for (int i = 20; i < 30; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    expected.push_back(tp);
  }

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(expected, expected);

	CREATE_DATA_LINK(op, mockup);

  for (int i = 20; i < 30; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    testTable->insert(i, *tp);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(mockup->numTuplesProcessed() == 10);
  testTable->drop();
}
