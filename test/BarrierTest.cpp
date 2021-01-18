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

#include <chrono>
#include <future>
#include <vector>

#include "StreamMockup.hpp"
#include "core/Tuple.hpp"
#include "qop/Barrier.hpp"
#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/Queue.hpp"

using namespace pfabric;

typedef TuplePtr<int> MyTuplePtr;

struct BarrierCounter {
  std::atomic<int> counter;
  std::condition_variable cVar;
  std::mutex mtx;

  BarrierCounter() { counter.store(0); }

  void set(int v) {
    std::unique_lock<std::mutex> lock(mtx);
    counter.store(v);
    cVar.notify_one();
  }

  int get() const { return counter.load(); }
};

/**
 * A simple test of the barrier operator.
 */
TEST_CASE("Controlling stream processing by a barrier", "[Barrier]") {
  using namespace std::chrono_literals;

  std::vector<MyTuplePtr> input = {
      makeTuplePtr(1),  makeTuplePtr(2),  makeTuplePtr(3),
      makeTuplePtr(4),  makeTuplePtr(11), makeTuplePtr(12),
      makeTuplePtr(20), makeTuplePtr(21), makeTuplePtr(22)};
  std::vector<MyTuplePtr> expected = {makeTuplePtr(1), makeTuplePtr(2),
                                      makeTuplePtr(3), makeTuplePtr(4)};

  BarrierCounter counter;

  counter.set(10);

  auto mockup =
      std::make_shared<StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);

  auto ch = std::make_shared<Queue<MyTuplePtr> >();

  auto barrier = std::make_shared<Barrier<MyTuplePtr> >(
      counter.cVar, counter.mtx,
      [&](auto tp) -> bool { return getAttribute<0>(tp) < counter.get(); });

  CREATE_DATA_LINK(mockup, ch)
  CREATE_DATA_LINK(ch, barrier)
  CREATE_DATA_LINK(barrier, mockup)

  // set counter to 10 and send tuples 1, 2, 3, 4, 11, 12:
  // => only tuples 1, 2, 3, 4 should arrive
  mockup->start();
  mockup->wait();
  REQUIRE(mockup->numTuplesProcessed() == 4);

  // now set counter to 13:
  // => we expect 11, 12 as results
  mockup->addExpected({makeTuplePtr(11), makeTuplePtr(12)});
  counter.set(13);

  mockup->wait();
  REQUIRE(mockup->numTuplesProcessed() == 6);

  // set counter to 25:
  // => we receive 20, 21, 22
  mockup->addExpected({makeTuplePtr(20), makeTuplePtr(21), makeTuplePtr(22)});
  counter.set(25);

  mockup->wait();
  REQUIRE(mockup->numTuplesProcessed() == 9);
}
