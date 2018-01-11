/*
 * Copyright (c) 2014-18 The PipeFabric team,
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

#include "catch.hpp"

#include <boost/fusion/include/set.hpp>
#include <boost/fusion/include/at_key.hpp>

#include "pubsub/Flow.hpp"

// some test data
static const int testNumerator = 3;
static const int testDenominator = 2;

// expected test results as different types
typedef boost::fusion::set< int, double > TestResults;
static const TestResults testResults( 1, 1.5 );

/**
 * @brief A simple sink for verifying test results.
 */
template< typename ResultType >
class DivResultChecker :
	public Sink<
		channels::In< const ResultType& >
	>
{
private:
	typedef Sink<
		channels::In< const ResultType& >
	> SinkBase;

	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 0, Result );

public:
	BIND_INPUT_CHANNEL_DEFAULT( Result, DivResultChecker, checkResult );

private:

	void checkResult( const ResultType& result ) {
		REQUIRE((result == boost::fusion::at_key< ResultType >(testResults)));
	}
};


/**
 * @brief A simple data flow calculating the division of two incoming integer numbers.
 */
class TestDiv :
	public Flow<
		  channels::SyncIn< int >  // one input channel for numerator    (will become IN 0)
		, channels::Out< int >     // outgoing division result as int    (will become OUT 0)
		, channels::In< int >      // one input channel for denomiator   (will become IN 1)
		, channels::Out< double >  // outgoing division result as double (will become OUT 1)
	>
{
private:

	// need an alias for the flow which is both, a source and a sink
	// -> same macros can be reused
	typedef Flow<
		  channels::SyncIn< int >
	 	, channels::Out< int >
		, channels::In< int >
		, channels::Out< double >
	> FlowBase;

	// assign alias for input channel types inherited from the sink side
	IMPORT_INPUT_CHANNEL_TYPE( FlowBase, 0, Numerator );
	IMPORT_INPUT_CHANNEL_TYPE( FlowBase, 1, Denominator );

public:

	TestDiv() :
		mWaitingForNumerator( true ),
		mWaitingForDenominator( true ),
		mNumerator(0),
		mDenominator(0) {
	}

	// bind callbacks for each input channel
	BIND_INPUT_CHANNEL_DEFAULT( Numerator, TestDiv, processNumerator );
	BIND_INPUT_CHANNEL_DEFAULT( Denominator, TestDiv, processDenominator );

private:

	void processNumerator( int n ) {
		mNumerator = n;
		mWaitingForNumerator = false;
		publishResult();
	}

	void processDenominator( int d ) {
		mDenominator = d;
		mWaitingForDenominator = false;
		publishResult();
	}

	void publishResult() {
		if( !mWaitingForNumerator && !mWaitingForDenominator ) {
			// ignore div 0
			PUBLISH( 0, mNumerator/mDenominator );
			PUBLISH( 1, static_cast< double >(mNumerator)/mDenominator );
			mWaitingForNumerator = true;
			mWaitingForDenominator = true;
		}
	}


	bool mWaitingForNumerator;   /**< flag for synchronizing missing values */
	bool mWaitingForDenominator; /**< flag for synchronizing missing values */
	int mNumerator;              /**< last numerator that was received */
	int mDenominator;            /**< last denominator that was received */
};


TEST_CASE( "Simple calculation via a data flow", "[flow]" ) {
	// create a data flow and some sinks
	TestDiv div;
	DivResultChecker< int > intResChecker;
	DivResultChecker< double > doubleResChecker;

	// connect them
	connectChannels( div.getOutputChannelByID<0>(), intResChecker.getInputChannelByID<0>() );
	connectChannels( div.getOutputChannelByID<1>(), doubleResChecker.getInputChannelByID<0>() );

	// publish some data directly through the flow's input channels
	auto& numerator = div.getInputChannelByID<0>();
	auto& denominator = div.getInputChannelByID<1>();
	numerator.getSlot()( testNumerator );
	denominator.getSlot()( testDenominator );
}
