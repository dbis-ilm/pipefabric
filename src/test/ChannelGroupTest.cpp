/*
v * ChannelGroupTest.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/equal.hpp>
#include <iostream>

#include "pubsub/channels/ChannelGroup.hpp"
#include "pubsub/Sink.hpp"
#include "pubsub/signals/StdSlot.hpp"
#include "pubsub/signals/BoostSlot.hpp"

#include "libcpp/utilities/GetTypeName.hpp"
#include "libcpp/utilities/TypePrinter.hpp"


struct TestComp1 {};
struct TestComp2 {};

TEST_CASE( "Verifying the work of channel groups", "[ChannelGroup]" ) {
	std::cout << "begin testChannelGroup" << std::endl;
	using namespace ns_utilities;
	using namespace boost::mpl;

	typedef Channel< ChannelID<0>, TestComp1, false, boost::mpl::vector< int > > Channel0_1;
	typedef Channel< ChannelID<0>, TestComp2, false, boost::mpl::vector< char > > Channel0_2;
	typedef Channel< ChannelID<1>, TestComp2, false, boost::mpl::vector< int*, const char& > > Channel1;

	typedef channel_group::EmptyChannelGroup emptyGrp;
	typedef typename channel_group::addChannel< emptyGrp, Channel0_1 >::type grpWithChannel0_1;
	typedef typename channel_group::addChannel< grpWithChannel0_1, Channel1 >::type grpWith_Channel_0_1_And_Channel_1;

	// must not compile due to duplicate ID
//	typedef typename channel_group::addChannel< grpWithChannel0_1, Channel0_2 >::type grpWithAllChannels;


	typedef typename channel_group::getChannel< grpWith_Channel_0_1_And_Channel_1, ChannelID<1>	>::type C1;

	std::cout << getTypeName< Channel1 >() << std::endl;
	std::cout << getTypeName< C1 >() << std::endl;

	BOOST_MPL_ASSERT_MSG(
		(boost::is_same< Channel1, C1 >::value),
		EXPECTED_CHANNEL_1,
		(Channel1, C1)
	);


	typedef typename channel_group::getChannels< grpWith_Channel_0_1_And_Channel_1 >::type allChannels;

	TypePrinter::apply< allChannels >();
	std::cout << std::endl;

	std::cout << "end testChannelGroup" << std::endl;
}


TEST_CASE( "Verifying the work of input channel groups", "[ChannelGroup]" ) {
	std::cout << "begin testInputChannelGroup" << std::endl;
	using namespace boost::mpl;
	using namespace ns_utilities;

	typedef vector<
		channels::In< int, char >,
		InputChannelParameters< true, BoostSlot, char, char >,
		channels::SyncIn< int, char, double >
	> ChannelParameters;

	typedef typename channel_group::generateChannelGroup<
		typename lambda< ::impl::createInputChannelType< TestComp1, boost::mpl::_1, boost::mpl::_2 > >::type,
		ChannelParameters
	>::type InputChannelGroup;
	typedef typename channel_group::getChannels< InputChannelGroup >::type GroupChannels;

	typedef vector3<
		InputChannel<
			ChannelID<0>,
			TestComp1,
			vector2< int, char >,
			DefaultSlotFunction< int, char >
		>,
		InputChannel<
			ChannelID<1>,
			TestComp1,
			vector2< char, char >,
			SynchronizedSlot< BoostSlot< char, char > >
		>,
		InputChannel<
			ChannelID<2>,
			TestComp1,
			vector3< int, char, double >,
			SynchronizedSlot< DefaultSlotFunction< int, char, double > >
		>
	> ExpectedChannels;

	std::cout << "expected channel list:" << std::endl;
	TypePrinter::apply< ExpectedChannels >();
	std::cout << std::endl;

	std::cout << "group channel list:" << std::endl;
	TypePrinter::apply< GroupChannels >();
	std::cout << std::endl;

	BOOST_MPL_ASSERT(( equal< GroupChannels, ExpectedChannels > ));

	std::cout << "end testInputChannelGroup" << std::endl;
}
