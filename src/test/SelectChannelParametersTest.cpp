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
