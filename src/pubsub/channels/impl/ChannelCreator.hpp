/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

/*
 * ChannelCreator.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef CHANNELCREATOR_HPP_
#define CHANNELCREATOR_HPP_


namespace impl {

/**
 * @brief Functor for creating new channel instances in a boost::fusion map.
 *
 * This functor is applied during construction of a channel @c Endpoint on
 * each entry in a boost::fusion map having the channel type as key and a
 * channel instance as value. The functor just creates a new channel instance
 * and stores it as mapped value for the entry.
 *
 * @tparam Endpoint
 *           the endpoint component the new channel instances are bound to
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename Endpoint
>
class ChannelCreator {
public:

	/**
	 * @brief Create a new generator for @c InputChannel instances.
	 *
	 * @param[in] processor the @c ChannelProcessor the new channels are bound to
	 */
	ChannelCreator( Endpoint& endpoint ) :
		mEndpoint( endpoint ) /*, mCount(0) */
	{}

	/**
	 * @brief The channel creation function.
	 *
	 * When this function is applied on a @c ChannelEntry, it will update its
	 * @c second value with a new channel instance. The actual
	 * @c InputChannel type is determined by the pair's @c first entry.
	 *
	 * @tparam ChannelEntry
	 *           a @c boost::fusion::pair representing an instance entry for
	 *           a specific channel, comprising the channel type (@c first_type)
	 *           and a channel instance (@c second)
	 * @param[in] channelEntry
	 *           a reference to the channel instance entry which should be
	 *           updated with a new channel instance
	 */
	template< typename ChannelEntry >
	void operator() ( ChannelEntry& channelEntry ) const {
		// TODO remove
		// std::cout << "creating channel " << mCount++ << std::endl;

		// the key entry in the map represents the actual channel type
		typedef typename ChannelEntry::first_type Channel;

		// update the value entry with a fresh channel instance bound to the endpoint
		channelEntry.second = std::move( Channel::create( mEndpoint ) );
	}

	Endpoint& mEndpoint; /**< the endpoint the new channels are bound to */
	// mutable unsigned int mCount; // TODO remove this
};

} /* end namespace impl */


#endif /* CHANNELCREATOR_HPP_ */
