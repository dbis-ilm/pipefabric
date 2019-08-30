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

#include "Topology.hpp"

using namespace pfabric;

Topology::~Topology() {
  if (!wakeupTimers.empty()) {
    for (auto& thr : wakeupTimers) {
      thr.interrupt();
    }
    for (auto& thr : wakeupTimers) {
      thr.join();
    }
  }
}

void Topology::registerStartupFunction(StartupFunc func) {
  startupList.push_back(func);
}

void Topology::cleanStartupFunctions() {
  startupFutures.clear();
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

void Topology::wait(const std::chrono::milliseconds &dur) {
    if (!asyncStarted)
      return;

    std::lock_guard<std::mutex> guard(mMutex);
    // let's wait until the function finished
    for(auto &f : startupFutures)
      f.get();
    //TODO: wait for EndOfStream Punctuations on all sinks
    //TODO: what about merging streams or no actual sinks?
    std::unique_lock<std::mutex> lk(mCv_m);
    const auto now = std::chrono::system_clock::now();
    if (mCv.wait_until(lk, now + dur) == std::cv_status::timeout) {
      //Timeout!
    }
}

void Topology::runEvery(unsigned long secs) {
  wakeupTimers.push_back(boost::thread([this, secs](){
        while(true) {
          boost::this_thread::sleep_for(boost::chrono::nanoseconds(secs));
          this->start(false);
        }
  }));
}

void Topology::stopThreads() {
  if (!wakeupTimers.empty()) {
    for (auto &thr : wakeupTimers) {
      thr.interrupt();
    }
    for (auto &thr : wakeupTimers) {
      thr.join();
    }
  }
}
    
Pipe<TStringPtr> Topology::newStreamFromFile(const std::string& fname, unsigned long limit) {
  // create a new TextFileSource
  auto op = std::make_shared<TextFileSource>(fname, limit);
  // register it's start function
  registerStartupFunction(std::bind(&TextFileSource::start, op.get()));
  // and create a new pipe
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}

#ifdef USE_RABBITMQ
Pipe<TStringPtr> Topology::newStreamFromRabbitMQ(const std::string& info, const std::string& queueName) {
  // create a new RabbitMQSource
  auto op = std::make_shared<RabbitMQSource>(info, queueName);
  // register it's start function
  registerStartupFunction(std::bind(&RabbitMQSource::start, op.get()));
  // and create a new pipe
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
  // and create a new pipe
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}
#endif

#ifdef USE_MQTT
Pipe<TStringPtr> Topology::newStreamFromMQTT(const std::string& conn, const std::string& channel) {
  // create a new MQTTSource
  auto op = std::make_shared<MQTTSource>(conn, channel);
  // register it's start function
  registerStartupFunction(std::bind(&MQTTSource::start, op.get()));
  // and create a new pipe
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
  // and create a new pipe
  return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}

Pipe<TStringPtr> Topology::newAsciiStreamFromZMQ(const std::string& path, const std::string& syncPath,
                                 ZMQParams::SourceType stype) {
    auto op = std::make_shared<ZMQSource<TStringPtr> >(path, syncPath, stype);
    registerStartupFunction(std::bind(&ZMQSource<TStringPtr>::start, op.get()));
    return Pipe<TStringPtr>(dataflow, dataflow->addPublisher(op));
}

Pipe<TBufPtr> Topology::newBinaryStreamFromZMQ(const std::string& path, const std::string& syncPath,
                                 ZMQParams::SourceType stype) {
    auto op = std::make_shared<ZMQSource<TBufPtr> >(path, syncPath, stype);
    registerStartupFunction(std::bind(&ZMQSource<TBufPtr>::start, op.get()));
    return Pipe<TBufPtr>(dataflow, dataflow->addPublisher(op));
}
