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
