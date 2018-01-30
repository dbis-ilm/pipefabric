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
#include "qop/Queue.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, int, int > MyTuplePtr;

/**
 * A simple test of the queue operator.
 */
TEST_CASE("Decoupling producer and consumer via a queue", "[Queue]") {
  std::vector<MyTuplePtr> input = {
    makeTuplePtr(0, 0, 0),
    makeTuplePtr (1, 1, 10),
    makeTuplePtr(2, 2, 20) };
  
  std::vector<MyTuplePtr> expected = {
    makeTuplePtr(0, 0, 0),
    makeTuplePtr (1, 1, 10),
    makeTuplePtr(2, 2, 20) };
  
  auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);
  
  auto ch = std::make_shared<Queue<MyTuplePtr> >();

  CREATE_DATA_LINK(mockup, ch)
  CREATE_DATA_LINK(ch, mockup)
    
  mockup->start();
  
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}
