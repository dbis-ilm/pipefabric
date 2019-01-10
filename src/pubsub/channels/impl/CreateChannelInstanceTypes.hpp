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

/*
 * CreateChannelInstanceTypes.hpp
 *
 *  Created on: Jun 4, 2014
 *      Author: fbeier
 */

#ifndef CREATECHANNELINSTANCETYPES_HPP_
#define CREATECHANNELINSTANCETYPES_HPP_


#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/transform.hpp>


namespace impl {

/**
 * @brief Meta function returning the nested instance type of a channel.
 *
 * This meta function can be invoked on a channel type which is passed as
 * @c Channel traits argument. It has to provide a nested @c Instance type.
 *
 * @tparam Channel
 *           the traits class for the channel type whose @c Instance type is requested
 */
template<
	typename Channel
>
struct GetChannelInstanceType {
	// return the nested Instance type
	typedef typename Channel::Instance type;
};


/**
 * @brief Meta function returning a fusion map for a list of @c Channels.
 *
 * This meta function can be invoked on a sequence of channel types, i.e, sequence
 * comprising unique channel type entries. It generates a @c boost::fusion::map
 * which can be used to actually store channel instances during runtime,
 * one for each channel type. This is required for generating @c Source and
 * @c Sink interface types.
 *
 * @tparam Channels
 *           a sequence comprising all channel types
 */
template<
	typename Channels
>
struct CreateChannelInstanceTypes {
	// generate a fusion map type out of the channel list
	typedef typename boost::fusion::result_of::as_map<
		// generate a new type vector storing a fusion pair for each
		// original type entry comprising the actual channel type as
		// first (key) entry and the instance type as second (value) entry
		typename boost::mpl::transform<
			Channels,  // a list of channels which is transformed in a new type
			boost::fusion::pair< // for each element, construct a pair
				boost::mpl::_1,  // taking the original type as first entry
				// and the nested instance type as second entry
				GetChannelInstanceType< boost::mpl::_1 >
			>
		>::type
	>::type type;
};

} /* end namespace impl */


#endif /* CREATECHANNELINSTANCETYPES_HPP_ */
