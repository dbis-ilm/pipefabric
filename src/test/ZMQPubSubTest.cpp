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

#include <sstream>
#include <thread>
#include <chrono>
#include <future>
#include <boost/filesystem.hpp>

#include <zmq.hpp>

#include "core/Tuple.hpp"
#include "qop/ZMQSource.hpp"
#include "qop/ZMQSink.hpp"
#include "qop/ConsoleWriter.hpp"
#include "qop/TupleExtractor.hpp"
#include "qop/TupleDeserializer.hpp"

#include "StreamMockup.hpp"

#include "fmt/format.h"

using namespace pfabric;

typedef TuplePtr< int, double, std::string > MyTuplePtr;

/**
 * A simple test of the ZMQSource/ZMQSink operators.
 */
TEST_CASE("Transfer a binary tuple stream via ZMQ", "[ZMQSource][ZMQSink]") {
	const int numTuples = 10000;

	std::vector<MyTuplePtr> input;

	for (int i = 0; i < numTuples; i++) {
		input.push_back(makeTuplePtr(i, i * 1.1, fmt::format("text{0}", i)));
	}

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, input);
	auto sink = std::make_shared< ZMQSink<MyTuplePtr> > ("tcp://*:5678");
	CREATE_DATA_LINK(mockup, sink);

	auto src = std::make_shared< ZMQSource<TBufPtr> > ("tcp://localhost:5678");
	auto deserializer = std::make_shared<TupleDeserializer<MyTuplePtr> >();
	CREATE_DATA_LINK(src, deserializer);
	CREATE_DATA_LINK(deserializer, mockup);

	using namespace std::chrono_literals;
	//std::this_thread::sleep_for(2s);

	auto handle = std::async(std::launch::async, [&mockup](){
		mockup->start();
	});

  handle.get();
  std::this_thread::sleep_for(2s);
  src->stop();
  REQUIRE(mockup->numTuplesProcessed() == numTuples);
}
