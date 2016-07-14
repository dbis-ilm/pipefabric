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

Pipe& Topology::newStreamFromREST(unsigned int port, const std::string& path, RESTSource::RESTMethod method, unsigned short numThreads) {
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

Pipe& Topology::newStreamFromZMQ(const std::string& path, ZMQParams::EncodingMode encoding, ZMQParams::SourceType stype) {
  Pipe *pipe = nullptr;
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

