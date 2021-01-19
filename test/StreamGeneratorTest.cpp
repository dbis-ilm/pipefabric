/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/StreamGenerator.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, int, int > MyTuplePtr;

/**
 * A simple test of the StreamGenerator operator.
 */
TEST_CASE("Producing a data stream using the StreamGenerator operator", "[StreamGenerator]") {

	std::vector<MyTuplePtr> expected;
  for (int i = 0; i < 1000; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    expected.push_back(tp);
  }

  auto op = std::make_shared<StreamGenerator<MyTuplePtr> >([](unsigned long n) -> MyTuplePtr {
      return makeTuplePtr((int)n, (int)n + 10, (int)n + 100);
      }, 1000);
	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(expected, expected);
	CREATE_DATA_LINK(op, mockup);

  op->start();
  REQUIRE(mockup->numTuplesProcessed() == 1000);
}

