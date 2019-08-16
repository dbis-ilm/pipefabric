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

#include "cppkafka/consumer.h"
#include "cppkafka/configuration.h"
#include "KafkaSource.hpp"

using namespace pfabric;

KafkaSource::KafkaSource(const std::string& broker, const std::string& topic,
                         const std::string& groupID) {
  //create config file, just necessary to start the consumer
  cppkafka::Configuration config = {
    { "metadata.broker.list", broker },
    { "group.id", groupID },
    { "enable.auto.commit", false }
  };

  consumer = new cppkafka::Consumer(config);
  consumer->subscribe({ topic });

  //(!) without polling once, we will not get the current position of data in the topic
  msg = consumer->poll();
}

KafkaSource::~KafkaSource() {
}

unsigned long KafkaSource::start() {
  //refresh (poll)
  msg = consumer->poll();

  //as long as there are messages - also possible to use "while(true)" to stay connected
  //getting messages sent later on, but for the test case we have to finish some time
  while(msg) {
    //no error handling currently, but there is also the error "eof" from Kafka that is
    //not necessary to handle here
    if (!msg.get_error()) {
      produceTuple(StringRef((char*)msg.get_payload().get_data(),
                             msg.get_payload().get_size()));
      consumer->commit(msg);
    }
    //check if there are more messages waiting
    msg = consumer->poll();
  }
  delete consumer;
  return 0;
}

void KafkaSource::stop() {
}

void KafkaSource::produceTuple(const StringRef& data) {
  auto tn = makeTuplePtr(data);
  this->getOutputDataChannel().publish(tn, false);
}

void KafkaSource::producePunctuation(PunctuationPtr pp) {
  this->getOutputPunctuationChannel().publish(pp);
}
