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

/*
 * ConnectChannels.hpp
 *
 *  Created on: Feb 9, 2015
 *      Author: fbeier
 */

#ifndef CONNECTCHANNELS_HPP_
#define CONNECTCHANNELS_HPP_

#include "ChannelTraits.hpp"
#include "PublisherTraits.hpp"
#include <boost/mpl/if.hpp>


/**
 * @brief Establish a link between two channels.
 *
 * This method connects two channels with each other, i.e., it creates a
 * @c Subscription for the data elements which are published through one of
 * them and subscribed by the other one. For doing this, one of the channels
 * must be an @c InputChannel and the other one an @c OutputChannel.
 * If two channels of the same type, i.e., both input or both output channels
 * are tried to be connected, a compiler error will be raised indicating this
 * situation.
 *
 * @tparam Channel1
 *           the type of the first channel (either @c InputChannel or @c OutputChannel),
 *           must be different as @c Channel2
 * @tparam Channel2
 *           the type of the second channel (either @c InputChannel or @c OutputChannel)
 *           must be different as @c Channel1
 */
template<
	typename Channel1,
	typename Channel2
>
SubscriptionPtr connectChannels( Channel1& channel1, Channel2& channel2 ) {
	using namespace boost::mpl;

	typedef ChannelTraits< Channel1 > Channel1Interface;
	typedef ChannelTraits< Channel2 > Channel2Interface;
	BOOST_STATIC_ASSERT_MSG(
		Channel1Interface::IS_INPUT_CHANNEL != Channel2Interface::IS_INPUT_CHANNEL,
		"cannot connect two input channels or two output channels with each other"
	);

	typedef typename if_c<
		!Channel1Interface::IS_INPUT_CHANNEL,
		PublisherTraits< Channel1 >,
		PublisherTraits< Channel2 >
	>::type Publisher;

	return Publisher::subscribe( channel1, channel2 );
}


#endif /* CONNECTCHANNELS_HPP_ */
