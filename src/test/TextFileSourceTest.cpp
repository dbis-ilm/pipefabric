#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <boost/filesystem.hpp>
#include <iostream>

// using namespace boost::filesystem;

#include "catch.hpp"

#include "fmt/format.h"

#include "core/Tuple.hpp"
#include "qop/TextFileSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "TestDataGenerator.hpp"

using namespace pfabric;
using namespace ns_types;

class TestConsumer : public SynchronizedDataSink<TStringPtr> {
public:
  PFABRIC_SYNC_SINK_TYPEDEFS(TStringPtr)

  TestConsumer() : tupleNum(0) {}

  BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, TestConsumer, processDataElement);
  BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, TestConsumer, processPunctuation);

  void processPunctuation(const PunctuationPtr& punctuation) {}

  void processDataElement( const TStringPtr& data, const bool outdated ) {
    std::string input (data->getAttribute<0>().begin_, data->getAttribute<0>().size_);
    std::string expected = fmt::format("{},This is a string field,{:.1f}", tupleNum, tupleNum * 100 + 0.5);

    REQUIRE(input == expected);
    tupleNum++;
  }

private:
  int tupleNum;
};

TEST_CASE("Reading a file", "[TextFileSource]" ) {
  // create a file of 1000 tuples (one tuple per line)
  TestDataGenerator tData("test.csv");
  tData.writeData(10000);

  auto fileSource = std::make_shared<TextFileSource>("test.csv");
  auto ntuples = fileSource->start();
  REQUIRE(ntuples == 10000);

  auto consumer = std::make_shared<TestConsumer>();
  CREATE_LINK(fileSource, consumer);
  fileSource->start();
}

#ifdef COMPRESSED_FILE_SOURCE
TEST_CASE("Reading a compressed file", "[TextFileSource]" ) {
  // create a file of 1000 tuples (one tuple per line)
  TestDataGenerator tData("test.csv");
  tData.writeData(10000, true);
  
  auto fileSource = std::make_shared<TextFileSource>("test.csv.gz");
  auto ntuples = fileSource->start();
  REQUIRE(ntuples == 10000);

  auto consumer = std::make_shared<TestConsumer>();
  CREATE_LINK(fileSource, consumer);
  fileSource->start();
}
#endif
