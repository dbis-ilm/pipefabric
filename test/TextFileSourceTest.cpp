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
