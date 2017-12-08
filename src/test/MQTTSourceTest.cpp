/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <string>
#include <iostream>

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"
#include "dsl/PFabricContext.hpp"

#include "mqtt/async_client.h"

using namespace pfabric;

TEST_CASE("Producing and receiving tuples via MQTT", "[MQTT]") {

  //prepare the consuming
  typedef TuplePtr<int, double> InTuplePtr;
  int resCntr = 0;

  PFabricContext ctx;
  auto t = ctx.createTopology();

  auto s = t->newStreamFromMQTT("tcp://localhost:1883", "test_topic")
    .extract<InTuplePtr>(',')
    .notify([&resCntr](auto tp, bool outdated) { resCntr++; })
    ;

  //start the producing
  mqtt::async_client client("tcp://localhost:1883", "producerID");

  mqtt::connect_options conopts;
  mqtt::token_ptr conntok = client.connect(conopts);
  conntok->wait();

  for(auto i=0; i<100; i++) {
    mqtt::message_ptr pubmsg = mqtt::make_message("test_topic", std::to_string(i)+",1.5");
    pubmsg->set_qos(1);
    client.publish(pubmsg)->wait_for(std::chrono::seconds(10));
  }
  conntok = client.disconnect();
  conntok->wait();

  //start the consuming
  t->start(false);

  REQUIRE(resCntr == 100);
}