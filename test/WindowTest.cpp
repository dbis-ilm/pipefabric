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

#include "catch.hpp"
#include <boost/core/ignore_unused.hpp>

#include <memory>
#include <set>

#include "core/Tuple.hpp"
#include "core/Punctuation.hpp"
#include "core/StreamElementTraits.hpp"
#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"
#include "qop/SlidingWindow.hpp"
#include "qop/TumblingWindow.hpp"


using namespace pfabric;

typedef TuplePtr<int, int, Timestamp> MyTuplePtr;

class TupleGenerator :
  public DataSource< MyTuplePtr >,
  public DataSink< MyTuplePtr >
{
public:
  typedef DataSource< MyTuplePtr > SourceBase;

  typedef DataSink< MyTuplePtr > SinkBase;
  typedef typename SinkBase::InputDataChannel InputDataChannel;
  typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel;
  typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

  TupleGenerator() :
    mTimestampExtractor(nullptr), mTuplesProcessed(0), mOutdatedTuplesProcessed(0) {
    }

  TupleGenerator(Window<MyTuplePtr>::TimestampExtractorFunc func) :
    mTimestampExtractor(func), mTuplesProcessed(0), mOutdatedTuplesProcessed(0) {
    }

  void start(int ntuples, Timestamp start_time = Timestamp(0)) {
    const bool outdated = false;
    mOutdatedTuplesProcessed = mTuplesProcessed = 0;
    for ( int i = 1; i <= ntuples; i++) {
      auto tp = makeTuplePtr(i, i, Timestamp(i * 1000000 + start_time.count()));
      this->getOutputDataChannel().publish(tp, outdated);
    }
  }

  int numProcessedTuples() const {
    return mTuplesProcessed;
  }

  int numOutdatedTuples() const {
    return mOutdatedTuplesProcessed;
  }


  /**
   * @brief Bind the callback for the data channel.
   */
  BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, TupleGenerator, processDataElement );

  /**
   * @brief Bind the callback for the punctuation channel.
   */
  BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, TupleGenerator, processPunctuation );


private:

  void processDataElement( const MyTuplePtr& data, const bool outdated ) {
    if (mTimestampExtractor != nullptr) {
      Timestamp ts = mTimestampExtractor( data );
      if (outdated) {
        // make sure that we have seen an outdated tuple already
        REQUIRE( mTupleSet.count( ts ) > 0);
        mOutdatedTuplesProcessed++;
      } else {
        mTuplesProcessed++;
        mTupleSet.insert( ts );
      }
    } else {
      if (outdated) mOutdatedTuplesProcessed++;
      else mTuplesProcessed++;
    }
  }

  void processPunctuation( const PunctuationPtr& punctuation ) {
    boost::ignore_unused( punctuation );
  }

  Window<MyTuplePtr>::TimestampExtractorFunc mTimestampExtractor;

  int mTuplesProcessed, mOutdatedTuplesProcessed;
  std::set<Timestamp> mTupleSet;
};


/**
 * A simple test of the row window operator.
 */
TEST_CASE("Checking a simple row-based sliding window", "[SlidingWindow]") {
  typedef SlidingWindow< MyTuplePtr > TestWindow;

  auto ts_fun = [&]( const MyTuplePtr& tp ) -> Timestamp {
    return tp->getAttribute<2>();
  };

  auto tgen = std::make_shared< TupleGenerator >(ts_fun);
  auto win = std::make_shared< TestWindow >( ts_fun, WindowParams::RowWindow, 10 );

  CREATE_DATA_LINK(tgen, win);
  CREATE_DATA_LINK(win, tgen);

  // we send 10 tuples to the window
  tgen->start(10);
  REQUIRE(tgen->numProcessedTuples() == 10);
  REQUIRE(tgen->numOutdatedTuples() == 0);

  // we send 10 more tuples
  tgen->start(10);
  REQUIRE(tgen->numProcessedTuples() == 10);
  // and we expect 10 outdated tuples
  REQUIRE(tgen->numOutdatedTuples() == 10);
}


TEST_CASE("Checking a simple range-based sliding window", "[SlidingWindow]") {
  typedef SlidingWindow< MyTuplePtr > TestWindow;

  auto ts_fun = [&]( const MyTuplePtr& tp ) -> Timestamp {
    return tp->getAttribute<2>();
  };

  auto tgen = std::make_shared<TupleGenerator>( ts_fun );
  auto win = std::make_shared< TestWindow >( ts_fun, WindowParams::RangeWindow, 10 );

  CREATE_DATA_LINK(tgen, win);
  CREATE_DATA_LINK(win, tgen);

  // we send 10 tuples to the window within 10 seconds
  tgen->start(10);
  REQUIRE(tgen->numProcessedTuples() == 10);
  REQUIRE(tgen->numOutdatedTuples() == 0);

  // we send 10 more tuples, but now with different start time
  tgen->start(10, Timestamp(11 * 1000000));
  REQUIRE(tgen->numProcessedTuples() == 10);
  // and we expect 10 outdated tuples
  REQUIRE(tgen->numOutdatedTuples() == 10);
}


TEST_CASE("Checking a row-based tumbling window", "[TumblingWindow]") {
  typedef TumblingWindow< MyTuplePtr > TestWindow;

  auto tgen = std::make_shared<TupleGenerator>();
  auto win = std::make_shared< TestWindow >( WindowParams::RowWindow, 3 );

  CREATE_DATA_LINK(tgen, win);
  CREATE_DATA_LINK(win, tgen);

  // we send 3 tuples to the window
  tgen->start(5);
  REQUIRE(tgen->numProcessedTuples() == 5);
  REQUIRE(tgen->numOutdatedTuples() == 3);

  // we send 5 more tuples
  tgen->start(5);
  REQUIRE(tgen->numProcessedTuples() == 5);
  // and we expect 6 outdated tuples
  REQUIRE(tgen->numOutdatedTuples() == 6);
}


TEST_CASE("Checking a range-based tumbling window", "[TumblingWindow]") {
  typedef TumblingWindow< MyTuplePtr > TestWindow;

  auto ts_fun = [&]( const MyTuplePtr& tp ) -> Timestamp {
    return tp->getAttribute<2>();
  };

  auto tgen = std::make_shared<TupleGenerator>( ts_fun );
  auto win = std::make_shared< TestWindow >(ts_fun, WindowParams::RangeWindow, 8 );

  CREATE_DATA_LINK(tgen, win);
  CREATE_DATA_LINK(win, tgen);

  // we send 10 tuples to the window within 10 seconds
  tgen->start(10);
  REQUIRE(tgen->numProcessedTuples() == 10);
  REQUIRE(tgen->numOutdatedTuples() == 8);
}
