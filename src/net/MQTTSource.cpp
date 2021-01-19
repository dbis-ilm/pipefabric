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

#include "mqtt/async_client.h"
#include "MQTTSource.hpp"
#include <string>

using namespace pfabric;

MQTTSource::MQTTSource(const std::string& conn, const std::string& channel) {
  chan = channel;

  mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(20);
	connOpts.set_clean_session(true);

  const int id = rand()%100000;
  cli = new mqtt::async_client(conn, std::to_string(id));

  cli->connect(connOpts)->wait();
  cli->start_consuming();
  cli->subscribe(channel, 1)->wait();
}

MQTTSource::~MQTTSource() {
}

unsigned long MQTTSource::start() {
  mqtt::const_message_ptr msg;

  //as long as there are messages to gather from server
  while(cli->try_consume_message(&msg)) {
    produceTuple(StringRef(msg->to_string().c_str(),
                           msg->to_string().size()));
  }
  cli->unsubscribe(chan)->wait();
  cli->stop_consuming();
  cli->disconnect()->wait();
  delete cli;
  return 0;
}

void MQTTSource::stop() {
}

void MQTTSource::produceTuple(const StringRef& data) {
  auto tn = makeTuplePtr(data);
  this->getOutputDataChannel().publish(tn, false);
}

void MQTTSource::producePunctuation(PunctuationPtr pp) {
  this->getOutputPunctuationChannel().publish(pp);
}
