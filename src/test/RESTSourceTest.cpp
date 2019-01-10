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

#include <boost/filesystem.hpp>
#include <iostream>
#include <future>

// using namespace boost::filesystem;

#include "catch.hpp"

#include "fmt/format.h"
#include "SimpleWeb/client_http.hpp"

#include "core/Tuple.hpp"
#include "qop/RESTSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

using namespace pfabric;
using namespace ns_types;

class TestConsumer : public SynchronizedDataSink<TStringPtr> {
public:
  PFABRIC_SYNC_SINK_TYPEDEFS(TStringPtr)

  TestConsumer() : tupleNum(0) {}

  BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, TestConsumer, processDataElement);
  BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, TestConsumer, processPunctuation);

  void processPunctuation(const PunctuationPtr& punctuation) {}

  void processDataElement( const TStringPtr& data, const bool outdated ) {
    std::string input (data->getAttribute<0>().begin_, data->getAttribute<0>().size_);
    std::string expected = fmt::format("(\"key\": \"{0}\",\"value\": \"Always the same\")", tupleNum);

    REQUIRE(input == expected);
    tupleNum++;
  }

private:
  int tupleNum;
};

typedef SimpleWeb::Client<SimpleWeb::HTTP> HttpClient;

TEST_CASE("Receiving data via REST", "[RESTSource]" ) {
  auto restSource = std::make_shared<RESTSource>(8099, "^/publish$", RESTSource::POST_METHOD);
  auto consumer = std::make_shared<TestConsumer>();
  CREATE_LINK(restSource, consumer);

  // note we have to start the REST server asynchronously
  auto handle = std::async(std::launch::async, [&](std::shared_ptr<RESTSource> src){ src->start(); }, restSource);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  HttpClient client("localhost:8099");
  for (int i = 0; i < 100; i++) {
    std::string param_string = fmt::format("(\"key\": \"{0}\",\"value\": \"Always the same\")", i);
    auto res = client.request("POST", "/publish", param_string);
  }
  restSource->stop();
  handle.get();

}
