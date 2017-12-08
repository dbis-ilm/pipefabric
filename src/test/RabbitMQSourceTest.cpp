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

#include "AMQPcpp.h"
#include <string>
#include <iostream>

#include "core/Tuple.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"
#include "dsl/PFabricContext.hpp"

using namespace pfabric;

TEST_CASE("Producing and receiving tuples via AMQP and RabbitMQ", "[RabbitMQ]") {

  std::cout<<"Producing 100 tuples..."<<std::endl;

  //produce
  AMQP amqp("guest:guest@localhost:5672"); //standard

  AMQPExchange* ex = amqp.createExchange("tupleProducer");
  ex->Declare("tupleProducer", "fanout");

  AMQPQueue *qu = amqp.createQueue("queue");
  qu->Declare();
  qu->Bind("tupleProducer","");

  std::string msg;

  for(auto i=0; i<100; i++) { //100 tuples
    msg = "";
    msg.append(std::to_string(i));
    msg.append(",1.5");
    ex->Publish(msg,"");
  }

  std::cout<<"Receiving..."<<std::endl;

  //consume
  typedef TuplePtr<int, double> InTuplePtr;
  int resCntr = 0;

  PFabricContext ctx;
  auto t = ctx.createTopology();

  auto s = t->newStreamFromRabbitMQ("guest:guest@localhost:5672", "queue")
    .extract<InTuplePtr>(',')
    .notify([&resCntr](auto tp, bool outdated) { resCntr++; })
    ;

  t->start(false);

  REQUIRE(resCntr == 100);
}
