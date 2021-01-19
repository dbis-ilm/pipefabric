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

#include <future>

#include "catch.hpp"
#include "fmt/format.h"
#include "SimpleWeb/client_http.hpp"

#include "qop/RESTSource.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;
using namespace ns_types;

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

TEST_CASE("Receiving data via REST", "[RESTSource]" ) {
  constexpr auto numTuples = 1000;
  std::vector<TStringPtr> expected;
  std::vector<std::string> stringVec;

  for (int i = 0; i < numTuples; i++) {
    auto param_string = fmt::format("(\"key\": \"{0}\",\"value\": \"Always the same\")", i);
    stringVec.push_back(std::move(param_string));
    expected.push_back(makeTuplePtr(StringRef(stringVec[i].c_str(), stringVec[i].size())));
  }

  auto restSource = std::make_shared<RESTSource>(8099, "^/publish$", RESTSource::POST_METHOD);
  auto mockup = std::make_shared<StreamMockup<TStringPtr, TStringPtr> >(expected, expected);
  CREATE_LINK(restSource, mockup);

  /// NOTE: we have to start the REST server asynchronously
  auto handle = std::async(std::launch::async, [&restSource, &stringVec](){
    restSource->start();
  });

  HttpClient client("localhost:8099");
  for (int i = 0; i < numTuples; i++) {
    auto res = client.request("POST", "/publish", stringVec[i]);
  }

  restSource->stop();
  handle.get();

  REQUIRE(mockup->numTuplesProcessed() == numTuples);
}
