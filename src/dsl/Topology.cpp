/*
 * Copyright (c) 2014-16 The PipeFabric team,
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


#include "Topology.hpp"
#include "qop/ZMQSource.hpp"

using namespace pfabric;

Topology::~Topology() {
  // delete all pipes we have created
  /*
  for (auto i : pipes) {
    delete i;
  }
  */
}

void Topology::registerStartupFunction(StartupFunc func) {
  startupList.push_back(func);
}

void Topology::registerPrepareFunction(StartupFunc func) {
  prepareList.push_back(func);
}

void Topology::startAsync() {
  // create futures for waiting for the results
  // of the start functions
  std::lock_guard<std::mutex> guard(mMutex);
  for (auto sFunc : startupList) {
    // make sure the function is launched asynchronously in a separate thread
    startupFutures.push_back(std::async(std::launch::async, sFunc));
  }
  asyncStarted = true;

}

void Topology::start(bool async) {
  if (async)
    startAsync();
  else
    for (auto sFunc : startupList) {
      (sFunc)();
    }
}

void Topology::prepare() {
    for (auto pFunc : prepareList) {
      (pFunc)();
    }
}

void Topology::wait() {
    if (!asyncStarted)
      return;

    std::lock_guard<std::mutex> guard(mMutex);
    // let's wait until the function finished
    for(auto &f : startupFutures)
      f.get();
}

Pipe<TStringPtr> Topology::newStreamFromFile(const std::string& fname, unsigned long limit) {
  // create a new TextFileSource
  auto op = std::make_shared<TextFileSource>(fname, limit);
  // register it's start function
  registerStartupFunction(std::bind(&TextFileSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}

#ifdef USE_RABBITMQ
Pipe<TStringPtr> Topology::newStreamFromRabbitMQ(const std::string& info, const std::string& queueName) {
  // create a new RabbitMQSource
  auto op = std::make_shared<RabbitMQSource>(info, queueName);
  // register it's start function
  registerStartupFunction(std::bind(&RabbitMQSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}
#endif

#ifdef USE_KAFKA
Pipe<TStringPtr> Topology::newStreamFromKafka(const std::string& broker, const std::string& topic,
                                              const std::string& groupID) {
  // create a new KafkaSource
  auto op = std::make_shared<KafkaSource>(broker, topic, groupID);
  // register it's start function
  registerStartupFunction(std::bind(&KafkaSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}
#endif

#ifdef USE_MQTT
Pipe<TStringPtr> Topology::newStreamFromMQTT(const std::string& conn, const std::string& channel) {
  // create a new MQTTSource
  auto op = std::make_shared<MQTTSource>(conn, channel);
  // register it's start function
  registerStartupFunction(std::bind(&MQTTSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}
#endif

Pipe<TStringPtr> Topology::newStreamFromREST(unsigned int port,
                                  const std::string& path,
                                  RESTSource::RESTMethod method,
                                  unsigned short numThreads) {
  // create a new RESTSource
  auto op = std::make_shared<RESTSource>(port, path, method, numThreads);
  // register it's start function
  registerStartupFunction(std::bind(&RESTSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}

Pipe<TStringPtr> Topology::newAsciiStreamFromZMQ(const std::string& path,
                                 ZMQParams::SourceType stype) {
    auto op = std::make_shared<ZMQSource<TStringPtr> >(path, stype);
    return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}

Pipe<TBufPtr> Topology::newBinaryStreamFromZMQ(const std::string& path,
                                 ZMQParams::SourceType stype) {
    auto op = std::make_shared<ZMQSource<TBufPtr> >(path, stype);
    return Pipe<TBufPtr>(dataflow, dataflow->addPublisher(op));
}
