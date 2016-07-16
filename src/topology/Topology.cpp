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
#include <vector>
#include <future>

#include "Topology.hpp"
#include "qop/ZMQSource.hpp"

using namespace pfabric;

Topology::~Topology() {
  // delete all pipes we have created
  for (auto i : pipes) {
    delete i;
  }
}

void Topology::registerStartupFunction(StartupFunc func) {
  startupList.push_back(func);
}

void Topology::startAsync() {
  // create futures for waiting for the results
  // of the start functions
  std::vector<std::future<unsigned long> > results;
  for (auto sFunc : startupList) {
    // make sure the function is launched asynchronously in a separate thread
    results.push_back(std::async(std::launch::async, sFunc));
  }
  // let's wait until the function finished
  for(auto &f : results)
    f.get();
}

void Topology::start(bool async) {
  if (async)
    startAsync();
  else
    for (auto sFunc : startupList) {
      (sFunc)();
    }
}

Pipe& Topology::newStreamFromFile(const std::string& fname) {
  // create a new TextFileSource
  auto op = std::make_shared<TextFileSource>(fname);
  // register it's start function
  registerStartupFunction(std::bind(&TextFileSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  auto s = new Pipe(op);
  pipes.push_back(s);
  return *s;
}

Pipe& Topology::newStreamFromREST(unsigned int port,
                                  const std::string& path,
                                  RESTSource::RESTMethod method,
                                  unsigned short numThreads) {
  // create a new TextFileSource
  auto op = std::make_shared<RESTSource>(port, path, method, numThreads);
  // register it's start function
  registerStartupFunction(std::bind(&RESTSource::start, op.get()));
  // and create a new pipe; we use a raw pointer here because
  // we want to return a reference to a Pipe object
  auto s = new Pipe(op);
  pipes.push_back(s);
  return *s;
}

Pipe& Topology::newStreamFromZMQ(const std::string& path,
                                 ZMQParams::EncodingMode encoding,
                                 ZMQParams::SourceType stype) {
  Pipe *pipe = nullptr;
  // depending on the encoding mode, we create a ZMQSource with
  // different type parameters
  if (encoding == ZMQParams::AsciiMode) {
    auto op = std::make_shared<ZMQSource<TStringPtr> >(path, stype);
    pipe = new Pipe(op);
  }
  else {
    auto op = std::make_shared<ZMQSource<TBufPtr> >(path, stype);
    pipe = new Pipe(op);
  }
  pipes.push_back(pipe);
  return *pipe;
}
