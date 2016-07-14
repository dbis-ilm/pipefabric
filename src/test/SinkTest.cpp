/*
 * SinkTest.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: fbeier
 */

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include "pubsub/Sink.hpp"
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


/**
 * @brief A simple test sink having an unsynchronized and a synchronized input channel.
 */
class TestSink :
	public Sink<
		  // utility generator for input channel parameters
		  channels::In< int, const TestData&, bool >
		, channels::SyncIn< TestData, double, const TestData* >
		  // explicit channel parameter type specialization
		, InputChannelParameters< false, StdSlot, const int* >
	>
{
	typedef Sink<
		  channels::In< int, const TestData&, bool >
		, channels::SyncIn< TestData, double, const TestData* >
		, InputChannelParameters< false, StdSlot, const int* >
	> Base;

	IMPORT_INPUT_CHANNEL_TYPE( Base, 0, FirstInputChannel );
	IMPORT_INPUT_CHANNEL_TYPE( Base, 1, SecondInputChannel );
	IMPORT_INPUT_CHANNEL_TYPE( Base, 2, ThirdInputChannel );

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
	BIND_INPUT_CHANNEL_DEFAULT( ThirdInputChannel, TestSink, processThirdChannel );

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

	void processThirdChannel( const int* iPtr ) {
		REQUIRE( iPtr == &testInt );
	}
};


TEST_CASE("Verifying the correct work of a sink", "[Sink]") {
	// create a test source having two input channels
	TestSink sink;

	// get a reference to its channels
	auto& in0 = sink.getInputChannelByID<0>();
	auto& in1 = sink.getInputChannelByID<1>();
	auto& in2 = sink.getInputChannelByID<2>();

	// and extract their underlying slots
	auto slot0 = in0.getSlot();
	auto slot1 = in1.getSlot();
	auto slot2 = in2.getSlot();

	// invoke the slots manually with pre-defined test data values
	slot0( testInt, testData, testBool );
	slot1( testData, testDouble, testDataPtr );
	slot2( &testInt );
}
