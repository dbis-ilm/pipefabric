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

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/ToTable.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, std::string, int > MyTuplePtr;

/**
 * A simple test of the projection operator.
 */
TEST_CASE("Writing a data stream to a table", "[ToTable]") {
  auto testTable = std::make_shared<Table<MyTuplePtr::element_type, int>>("myTable22");

	std::vector<MyTuplePtr> input = {
		makeTuplePtr(0, std::string("String #1"), 0),
		makeTuplePtr (1, std::string("String #2"), 10),
		makeTuplePtr(2, std::string("String #3"), 20) };

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, input);

	auto keyFunc = [&](const MyTuplePtr& tp) -> int { return tp->getAttribute<0>(); };
	auto op = std::make_shared< ToTable<MyTuplePtr, int> >(testTable, keyFunc);

	CREATE_DATA_LINK(mockup, op)

	mockup->start();

	REQUIRE(testTable->size() == 3);

  for (int i = 0; i < 3; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == fmt::format("String #{0}", i+1));
    REQUIRE(get<2>(tp) == i * 10);
  }
  testTable->drop();
}
