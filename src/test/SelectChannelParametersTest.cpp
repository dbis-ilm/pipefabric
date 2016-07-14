/*
 * SelectChannelParametersTest.cpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/mpl/assert.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/at.hpp>

#include "pubsub/Sink.hpp"
#include "pubsub/Source.hpp"
#include "pubsub/channels/parameters/SelectInputChannelParameters.hpp"
#include "pubsub/channels/parameters/SelectOutputChannelParameters.hpp"


TEST_CASE("Verifying InputChannelParameter", "[ChannelParameter]" ) {
	BOOST_MPL_ASSERT_NOT(( isInputChannelParameter< char > ));
	BOOST_MPL_ASSERT(( isInputChannelParameter< channels::In< int > > ));
	BOOST_MPL_ASSERT(( isInputChannelParameter< channels::SyncIn< int > > ));
	BOOST_MPL_ASSERT(( isInputChannelParameter< InputChannelParameters< false, DefaultSlotFunction, int > > ));
	BOOST_MPL_ASSERT_NOT(( isInputChannelParameter< channels::Out< int > > ));
	BOOST_MPL_ASSERT_NOT(( isInputChannelParameter< OutputChannelParameters< DefaultSourceSignal, int > > ));
}


TEST_CASE("Verifying OutputChannelParameter", "[ChannelParameter]" ) {
	BOOST_MPL_ASSERT_NOT(( isOutputChannelParameter< char > ));
	BOOST_MPL_ASSERT_NOT(( isOutputChannelParameter< channels::In< int > > ));
	BOOST_MPL_ASSERT_NOT(( isOutputChannelParameter< channels::SyncIn< int > > ));
	BOOST_MPL_ASSERT_NOT(( isOutputChannelParameter< InputChannelParameters< false, DefaultSlotFunction, int > > ));
	BOOST_MPL_ASSERT(( isOutputChannelParameter< channels::Out< int > > ));
	BOOST_MPL_ASSERT(( isOutputChannelParameter< OutputChannelParameters< DefaultSourceSignal, int > > ));
}


TEST_CASE("Verifying SelectInputChannelsOnly", "[ChannelParameter]" ) {
	using namespace boost::mpl;

	typedef channels::In< int, double > FirstChannel;
	typedef channels::In< char, double > SecondChannel;
	typedef channels::SyncIn< char, double > ThirdChannel;

	typedef typename SelectInputChannelParameters<
		FirstChannel, SecondChannel, FirstChannel, ThirdChannel
	>::type Channels;

	BOOST_MPL_ASSERT(( equal< size< Channels >, int_< 4 > > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 0 >, FirstChannel > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 1 >, SecondChannel > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 2 >, FirstChannel > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 3 >, ThirdChannel > ));
}


TEST_CASE("Verifying SelectOutputChannelsOnly", "[ChannelParameter]" ) {
	using namespace boost::mpl;

	typedef channels::Out< int, double > FirstChannel;
	typedef channels::Out< char, double > SecondChannel;

	typedef typename SelectOutputChannelParameters<
		FirstChannel, SecondChannel, FirstChannel
	>::type Channels;

	BOOST_MPL_ASSERT(( equal< size< Channels >, int_< 3 > > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 0 >, FirstChannel > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 1 >, SecondChannel > ));
	BOOST_MPL_ASSERT(( equal< at_c< Channels, 2 >, FirstChannel > ));
}
