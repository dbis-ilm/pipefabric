#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <boost/filesystem.hpp>
#include <iostream>

#include "catch.hpp"

#include "fmt/format.h"

#include "core/Tuple.hpp"
#include "qop/MemorySource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "TestDataGenerator.hpp"

using namespace pfabric;

template <typename StreamElement>
class TestConsumer : public SynchronizedDataSink<StreamElement> {
public:
  PFABRIC_SYNC_SINK_TYPEDEFS(StreamElement)

  TestConsumer() : tupleNum(0) {}

  BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, TestConsumer, processDataElement);
  BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, TestConsumer, processPunctuation);

  void processPunctuation(const PunctuationPtr& punctuation) {}

  void processDataElement( const StreamElement& data, const bool outdated ) {
    auto expected = makeTuplePtr(tupleNum, std::string("This is a string field"), (double)(tupleNum) * 100.0 + 0.5);

    REQUIRE(*data == *expected);
    tupleNum++;
  }

  unsigned long numTuples() const { return tupleNum; }

private:
  int tupleNum;
};

typedef TuplePtr<int, std::string, double> MyTuple;

TEST_CASE("Preparing a MemorySource from a file", "[MemorySource]" ) {
  // create a file of 1000 tuples (one tuple per line)
  TestDataGenerator tData("test.csv");
  tData.writeData(10000);

  auto memSource = std::make_shared<MemorySource<MyTuple>>("test.csv");
  auto ntuples = memSource->prepare();
  REQUIRE(ntuples == 10000);

  auto consumer = std::make_shared<TestConsumer<MyTuple>>();
  CREATE_LINK(memSource, consumer);

  memSource->start();
  REQUIRE(consumer->numTuples() == 10000);
}

TEST_CASE("Preparing a MemorySource from a file with limit", "[MemorySource]" ) {
  // create a file of 1000 tuples (one tuple per line)
  TestDataGenerator tData("test.csv");
  tData.writeData(10000);

  auto memSource = std::make_shared<MemorySource<MyTuple>>("test.csv", ',', 100);
  auto ntuples = memSource->prepare();
  REQUIRE(ntuples == 100);

  auto consumer = std::make_shared<TestConsumer<MyTuple>>();
  CREATE_LINK(memSource, consumer);

  memSource->start();
  REQUIRE(consumer->numTuples() == 100);
}
