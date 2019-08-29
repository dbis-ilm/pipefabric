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

#include "StreamMockup.hpp"
#include "core/Tuple.hpp"
#include "qop/ZMQSource.hpp"
#include "qop/ConsoleWriter.hpp"
#include "qop/TupleExtractor.hpp"
#include "qop/TupleDeserializer.hpp"


using namespace pfabric;

using MyTuplePtr = TuplePtr<int, int>;

/**
 * A simple test of the ZMQSource operator.
 */
TEST_CASE("Receiving a ascii tuple stream via ZMQSource", "[ZMQSource]") {
  using TestZMQSource = ZMQSource<TStringPtr>;

  std::vector<MyTuplePtr> expected = {makeTuplePtr(0, 10), makeTuplePtr(1,11),
                                      makeTuplePtr(2, 12), makeTuplePtr(3,13),
                                      makeTuplePtr(4, 14), makeTuplePtr(5,15)};

  zmq::context_t context (1);
  zmq::socket_t publisher(context, ZMQ_PUB);
  publisher.bind("tcp://*:5678");
  zmq::socket_t syncservice(context, ZMQ_REP);
  syncservice.bind("tcp://*:5679");

  auto handle = std::async(std::launch::async, [&publisher, &syncservice](){
    std::vector<std::string> input = {
      "0|10", "1|11", "2|12", "3|13", "4|14", "5|15"
    };
    
    zmq::message_t message;
    syncservice.recv(message, zmq::recv_flags::none);
    
    for(const std::string &s : input) {
      zmq::message_t request(s.length());
      memcpy(request.data(), s.c_str(), s.length());
      publisher.send(request, zmq::send_flags::none);
    }
  });

  auto src = std::make_shared< TestZMQSource > ("tcp://localhost:5678", "tcp://localhost:5679");
  auto extractor = std::make_shared<TupleExtractor<MyTuplePtr>>('|');
  auto mockup = std::make_shared<StreamMockup<MyTuplePtr, MyTuplePtr>>(expected, expected);

  CREATE_DATA_LINK(src, extractor);
  CREATE_DATA_LINK(extractor, mockup);

  handle.get();
  mockup->wait();
  src->stop();

  REQUIRE(mockup->numTuplesProcessed() == 6);

  publisher.close();
  syncservice.close();
}

/**
 * A second test of the ZMQSource operator.
 */
TEST_CASE("Receiving a binary tuple stream via ZMQSource", "[ZMQSource]") {
  using TestZMQSource = ZMQSource<TBufPtr>;

  std::vector<MyTuplePtr> expected = {makeTuplePtr(0, 10), makeTuplePtr(1,11),
                                      makeTuplePtr(2, 12), makeTuplePtr(3,13),
                                      makeTuplePtr(4, 14), makeTuplePtr(5,15)};

  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("tcp://*:5678");
  zmq::socket_t syncservice(context, ZMQ_REP);
  syncservice.bind("tcp://*:5679");

  auto handle = std::async(std::launch::async, [&expected, &publisher, &syncservice](){
    zmq::message_t message;
    syncservice.recv(message, zmq::recv_flags::none);

    for(MyTuplePtr &tp : expected) {
      StreamType res;
      tp->serializeToStream(res);
      zmq::message_t request (res.size());
      memcpy (request.data (), res.data(), res.size());
      publisher.send(request, zmq::send_flags::none);
    }
  });

  auto src = std::make_shared< TestZMQSource > ("tcp://localhost:5678", "tcp://localhost:5679");
  auto extractor = std::make_shared<TupleDeserializer<MyTuplePtr> >();
  auto mockup = std::make_shared<StreamMockup<MyTuplePtr, MyTuplePtr>>(expected, expected);

  CREATE_DATA_LINK(src, extractor);
  CREATE_DATA_LINK(extractor, mockup);

  handle.get();
  mockup->wait();
  src->stop();

  REQUIRE(mockup->numTuplesProcessed() == 6);

  publisher.close();
  syncservice.close();
}
