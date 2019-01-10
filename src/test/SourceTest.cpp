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

#include "catch.hpp"

#include "pubsub/Source.hpp"
#include "pubsub/Sink.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "pubsub/signals/BoostSignal.hpp"
#include "pubsub/signals/StdSlot.hpp"

#include <functional>
#include <string>
#include <vector>


/// a complex data type to be published
class TestData {
public:
	typedef std::vector< std::string > StringVec;

	TestData( const std::string& s1, const std::string& s2 ) :
		mIsCopy( false ) {
		mData.push_back( s1 );
		mData.push_back( s2 );
	}

	TestData( const TestData& other ) :
		mData( other.mData ), mIsCopy( true ) {
	}

	const StringVec& getData() const {
		return mData;
	}

	bool isCopy() const {
		return mIsCopy;
	}

private:
	StringVec mData;
	const bool mIsCopy;
};


// some test data to be received by the sink
static const TestData testData = TestData( "Hello", "World" );
static const TestData* testDataPtr = &testData;
static const bool testBool = false;
static const int testInt = 1;
static const double testDouble = 2.0;

// some global counters for tracking how many times a channel slot was invoked
static int numFirstChannelInvocations = 0;
static int numSecondChannelInvocations = 0;


/**
 * @brief A simple test sink having an unsynchronized and a synchronized input channel.
 */
class TestSink :
	public Sink<
		  channels::In< int, const TestData&, bool >
		, channels::SyncIn< TestData, double, const TestData* >
	>
{
	typedef Sink<
		  channels::In< int, const TestData&, bool >
		, channels::SyncIn< TestData, double, const TestData* >
	> Base;

	IMPORT_INPUT_CHANNEL_TYPE( Base, 0, FirstInputChannel );
	IMPORT_INPUT_CHANNEL_TYPE( Base, 1, SecondInputChannel );

public:

	TestSink() : Base( "TestSink" ) {}

	/**
	 * @brief Bind the callback for the first input channel.
	 *
	 * @param[in] c
	 *    a reference to input channel to be bound, required for dispatching
	 * @return a callback to be used by the channel
	 */
	virtual typename FirstInputChannel::Slot bindInputChannel( const FirstInputChannel& c ) override {
		return std::bind( &TestSink::processFirstChannel, this,
			std::ref( c ), // inject additional argument
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3
		);
	}

	/**
	 * @brief Bind the callback for the second input channel.
	 *
	 * Use the helper macro for generating boilerplate code that binds the second input
	 * channel to the @c processSecondChannel() member function which accepts arguments
	 * in the exact order as defined for the second input channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( SecondInputChannel, TestSink, processSecondChannel );

	/**
	 * @brief Handle incoming data elements from the first input channel.
	 *
	 * This method will be used as callback for incoming data elements from the sink's
	 * first input channel. It verifies that the incoming data matches the pre-defined
	 * test data which is passed to the channel in the test.
	 *
	 * @param[in] c
	 *     a reference to the input channel for testing that additional args can be
	 *     passed to the channel's callback slot
	 * @param[in] i
	 *     a const reference to a simple int value (is convertible from the original channel type int)
	 * @param[in] d
	 *     a complex data structure taken by reference -> shouldn't be copied
	 * @param[in] b
	 *     a simple bool value
	 */
	void processFirstChannel( const FirstInputChannel& c, const int& i, const TestData& d, bool b ) {
		numFirstChannelInvocations++; // count the slot invocation

		REQUIRE( i == testInt );

		REQUIRE( d.getData().size() == 2 );
		REQUIRE( d.getData()[0] == "Hello" );
		REQUIRE( d.getData()[1] == "World" );
		REQUIRE( d.isCopy() == false );

		REQUIRE( b == testBool );
	}

	/**
	 * @brief Handle incoming data elements from the second input channel.
	 *
	 * This method will be used as callback for incoming data elements from the sink's
	 * second input channel. It verifies that the incoming data matches the pre-defined
	 * test data which is passed to the channel in the test.
	 *
	 * @param[in] data
	 *     a complex data structure taken by value -> should be copied
	 * @param[in] d
	 *     a simple double value
	 * @param[in] dataPtr
	 *     a pointer to a complex data structure -> should point to the global variable
	 */
	void processSecondChannel( TestData data, double d, const TestData* dataPtr ) {
		numSecondChannelInvocations++; // count the slot invocation

		// data taken by value -> should be a copy with the same member values
		REQUIRE( data.getData().size() == 2 );
		REQUIRE( data.getData()[0] == "Hello" );
		REQUIRE( data.getData()[1] == "World" );
		REQUIRE( data.isCopy() == true );

		REQUIRE( d == testDouble );

		// dataPtr should point to the global variable which shouldn't be copied
		REQUIRE( dataPtr == testDataPtr );
		REQUIRE( dataPtr->getData()[0] == "Hello" );
		REQUIRE( dataPtr->getData()[1] == "World" );
		REQUIRE( dataPtr->isCopy() == false );
	}
};


/// manually defined signal
template< typename... Args >
using TestSignal = BoostSignal< StdSlot, Args... >;

/**
 * @brief A simple test source having two outgoing channels.
 */
class TestSource :
	public Source<
		  // default output channel generation
		  channels::Out< int, const TestData&, bool >
		, channels::Out< TestData, double, const TestData* >
		  // manually specified output channel parameter
		, OutputChannelParameters< TestSignal, char >
	>
{
private:
	typedef Source<
		  channels::Out< int, const TestData&, bool >
		, channels::Out< TestData, double, const TestData* >
		, OutputChannelParameters< TestSignal, char >
	> Base;

	IMPORT_OUTPUT_CHANNEL_TYPE( Base, 0, FirstOutputChannel );
	IMPORT_OUTPUT_CHANNEL_TYPE( Base, 1, SecondOutputChannel );

public:

	TestSource() :
		Base( "TestSource" ),
		mFirstOutChannel( getOutputChannelByID< 0 >() ) {
	}

	/**
	 * @brief Publish some test data through the first output channel.
	 *
	 * This method will invoke the @c publish() method of a cached reference
	 * to the first output channel.
	 */
	void publishFirst() {
		mFirstOutChannel.publish( testInt, testData, testBool );
	}

	/**
	 * @brief Publish some test data through the second output channel.
	 *
	 * This method will use the @c PUBLISH macro for invoking the @c publish()
	 * method of the second output channel.
	 */
	void publishSecond() {
		PUBLISH( 1, testData, testDouble, testDataPtr );
	}

private:

	FirstOutputChannel& mFirstOutChannel; /**< a cached reference to the first output channel */
};



/**
 * @brief Test case to verify subscription management interfaces.
 */
 TEST_CASE("Verifying subscription management interfaces", "[Source]") {
	// create a test source having two output channels
	TestSource source;

	// get a reference to its channels
	typedef typename TestSource::getOutputChannelTypeByID< 0 >::type Out0;
	typedef typename TestSource::getOutputChannelTypeByID< 1 >::type Out1;
	auto& out0 = source.getOutputChannelByID<0>();
	auto& out1 = source.getOutputChannelByID<1>();


	// create a test sink having two input channels
	TestSink sink;

	// get a reference to its channels
	typedef typename TestSink::getInputChannelTypeByID< 0 >::type In0;
	typedef typename TestSink::getInputChannelTypeByID< 1 >::type In1;
	auto& in0 = sink.getInputChannelByID<0>();
	auto& in1 = sink.getInputChannelByID<1>();


	// add a subscription for both
	auto subscription0 = in0.subscribe( out0 ); // input to output
	REQUIRE( in0.getNumSubscriptions() == 1 );
	REQUIRE( out0.getNumSubscriptions() == 1 );

	auto subscription1 = out1.subscribe( in1 );  // output to input
	REQUIRE( in1.getNumSubscriptions() == 1 );
	REQUIRE( out1.getNumSubscriptions() == 1 );


	// add another subscription for both, access via traits
	auto subscription2 = connectChannels( in0, out0 );
	REQUIRE( ChannelTraits< In0 >::getNumSubscriptions( in0 ) == 2 );
	REQUIRE( ChannelTraits< Out0 >::getNumSubscriptions( out0 ) == 2 );

	auto subscription3 = connectChannels( out1, in1 );
	REQUIRE( ChannelTraits< In1 >::getNumSubscriptions( in1 ) == 2 );
	REQUIRE( ChannelTraits< Out1 >::getNumSubscriptions( out1 ) == 2 );


	// explicitly close the subscriptions via their handles
	subscription0->close();
	REQUIRE( in0.getNumSubscriptions() == 1 );
	REQUIRE( out0.getNumSubscriptions() == 1 );
	REQUIRE( ChannelTraits< In1 >::getNumSubscriptions( in1 ) == 2 );
	REQUIRE( ChannelTraits< Out1 >::getNumSubscriptions( out1 ) == 2 );

	subscription2->close();
	REQUIRE( in0.getNumSubscriptions() == 0 );
	REQUIRE( out0.getNumSubscriptions() == 0 );
	REQUIRE( ChannelTraits< In1 >::getNumSubscriptions( in1 ) == 2 );
	REQUIRE( ChannelTraits< Out1 >::getNumSubscriptions( out1 ) == 2 );

	subscription1->close();
	REQUIRE( in0.getNumSubscriptions() == 0 );
	REQUIRE( out0.getNumSubscriptions() == 0 );
	REQUIRE( ChannelTraits< In1 >::getNumSubscriptions( in1 ) == 1 );
	REQUIRE( ChannelTraits< Out1 >::getNumSubscriptions( out1 ) == 1 );

	subscription3->close();
	REQUIRE( in0.getNumSubscriptions() == 0 );
	REQUIRE( out0.getNumSubscriptions() == 0 );
	REQUIRE( ChannelTraits< In1 >::getNumSubscriptions( in1 ) == 0 );
	REQUIRE( ChannelTraits< Out1 >::getNumSubscriptions( out1 ) == 0 );
}


/**
 * @brief Test case to verify that all subscriptions are closed when a sink is destroyed.
 */
TEST_CASE("Verifying that all subscriptions are closed when a sink is destroyed", "[Source]") {
	// create a test source having two output channels
	TestSource source;
	auto& out0 = source.getOutputChannelByID<0>();
	auto& out1 = source.getOutputChannelByID<1>();


	SubscriptionPtr sub0;
	SubscriptionPtr sub1;
	{ // new scope for auto deletion of test sink
		// create a test sink having two input channels
		TestSink sink;
		auto& in0 = sink.getInputChannelByID<0>();
		auto& in1 = sink.getInputChannelByID<1>();


		// add a subscription for both
		sub0 = connectChannels( in0, out0 );
		REQUIRE( in0.getNumSubscriptions() == 1 );
		REQUIRE( out0.getNumSubscriptions() == 1 );

		sub1 = connectChannels( in1, out1 );
		REQUIRE( in1.getNumSubscriptions() == 1 );
		REQUIRE( out1.getNumSubscriptions() == 1 );

		REQUIRE( sub0->isConnected() );
		REQUIRE( sub1->isConnected() );
	}

	REQUIRE( out0.getNumSubscriptions() == 0 );
	REQUIRE( out1.getNumSubscriptions() == 0 );
	REQUIRE( ! sub0->isConnected() );
	REQUIRE( ! sub1->isConnected() );
}


/**
 * @brief Test case to verify that all subscriptions are closed when a source is destroyed.
 */
TEST_CASE("Verifying that all subscriptions are closed when a source is destroyed", "[Source]") {
	// create a test sink having two input channels
	TestSink sink;
	auto& in0 = sink.getInputChannelByID<0>();
	auto& in1 = sink.getInputChannelByID<1>();

	SubscriptionPtr sub0;
	SubscriptionPtr sub1;
	{ // new scope for auto deletion of test source
		// create a test source having two output channels
		TestSource source;
		auto& out0 = source.getOutputChannelByID<0>();
		auto& out1 = source.getOutputChannelByID<1>();

		// add a subscription for both
		sub0 = connectChannels( in0, out0 );
		REQUIRE( in0.getNumSubscriptions() == 1 );
		REQUIRE( out0.getNumSubscriptions() == 1 );

		sub1 = connectChannels( in1, out1 );
		REQUIRE( in1.getNumSubscriptions() == 1 );
		REQUIRE( out1.getNumSubscriptions() == 1 );

		REQUIRE( sub0->isConnected() );
		REQUIRE( sub1->isConnected() );
	}

	REQUIRE( in0.getNumSubscriptions() == 0 );
	REQUIRE( in1.getNumSubscriptions() == 0 );
	REQUIRE( ! sub0->isConnected() );
	REQUIRE( ! sub1->isConnected() );
}


/**
 * @brief Test case to verify that nothing happens when a source publishes data without
 *        having any subscriptions.
 */
TEST_CASE("Publishing data by a source without having any subscriptions", "[Source]") {
	// create a test source having two output channels
	TestSource source;

	// publish via wrapper method
	numFirstChannelInvocations = 0;
	numSecondChannelInvocations = 0;
	source.publishFirst();
	source.publishSecond();
	REQUIRE( numFirstChannelInvocations == 0 );
	REQUIRE( numSecondChannelInvocations == 0 );

	// publish directly via channel
	source.getOutputChannelByID<0>().publish( testInt, testData, testBool );
	source.getOutputChannelByID<1>().publish( testData, testDouble, testDataPtr );
	REQUIRE( numFirstChannelInvocations == 0 );
	REQUIRE( numSecondChannelInvocations == 0 );
}


/**
 * @brief Test case to verify that data is correctly published by a source to subscribing sinks
 *        via their In- and OutputChannels.
 */
TEST_CASE("Publishing data by a source to subscribing sinks", "[Source]") {
	// create a test source having two output channels
	TestSource source;
	auto& out0 = source.getOutputChannelByID<0>();
	auto& out1 = source.getOutputChannelByID<1>();


	// create two test sinks having two input channels each
	TestSink sink0;
	auto& in00 = sink0.getInputChannelByID<0>();
	auto& in01 = sink0.getInputChannelByID<1>();

	TestSink sink1;
	auto& in10 = sink1.getInputChannelByID<0>();
	auto& in11 = sink1.getInputChannelByID<1>();


	// establish some connections
	connectChannels( in00, out0 );
	auto duplicate00 = connectChannels( in00, out0 );
	connectChannels( in01, out1 );
	auto duplicate01 = connectChannels( in01, out1 );

	connectChannels( out0, in10 );
	connectChannels( out1, in11 );

	REQUIRE( in00.getNumSubscriptions() == 2 );
	REQUIRE( in01.getNumSubscriptions() == 2 );
	REQUIRE( in10.getNumSubscriptions() == 1 );
	REQUIRE( in11.getNumSubscriptions() == 1 );
	REQUIRE( out0.getNumSubscriptions() == 3 );
	REQUIRE( out1.getNumSubscriptions() == 3 );


	// publish some data
	numFirstChannelInvocations = 0;
	numSecondChannelInvocations = 0;
	source.publishFirst();
	source.publishSecond();
	REQUIRE( numFirstChannelInvocations == 3 );
	REQUIRE( numSecondChannelInvocations == 3 );

	// disconnect some channels and publish again
	duplicate00->close();
	duplicate01->close();
	numFirstChannelInvocations = 0;
	numSecondChannelInvocations = 0;
	source.publishFirst();
	source.publishSecond();
	REQUIRE( numFirstChannelInvocations == 2 );
	REQUIRE( numSecondChannelInvocations == 2 );
}
