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

#include "AMQPcpp.h"
#include "RabbitMQSource.hpp"

using namespace pfabric;
  
RabbitMQSource::RabbitMQSource(const std::string& info, const std::string& queueName) {
  mInfo = info;
  mQueueName = queueName;
}

RabbitMQSource::~RabbitMQSource() {
}

unsigned long RabbitMQSource::start() {
  AMQP amqp(mInfo);
  AMQPQueue* que = amqp.createQueue(mQueueName);
  que->Declare();
  uint32_t len = 0;
  char* data;

  //get first tuple
  que->Get(AMQP_NOACK);
  AMQPMessage* m = que->getMessage();
  if(m->getMessageCount() > 0) {
    data = m->getMessage(&len);
    produceTuple(StringRef(data, len));
  }

  //and all the others
  while(m->getMessageCount() > 0) {
    que->Get(AMQP_NOACK);
    m = que->getMessage();
    data = m->getMessage(&len);
    produceTuple(StringRef(data, len));
  }
  return 0;
}

void RabbitMQSource::stop() {
}

void RabbitMQSource::produceTuple(const StringRef& data) {
  auto tn = makeTuplePtr(data);
  this->getOutputDataChannel().publish(tn, false);
}

void RabbitMQSource::producePunctuation(PunctuationPtr pp) {
  this->getOutputPunctuationChannel().publish(pp);
}
