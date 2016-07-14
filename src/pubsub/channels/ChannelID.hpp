/*
 * ChannelID.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef CHANNELID_HPP_
#define CHANNELID_HPP_

#include <boost/mpl/integral_c.hpp>

/// type for storing a unique channel ID
typedef unsigned int ChannelIDValue;

/**
 * @brief Compile-time constant for storing a unique ChannelID.
 *
 * The ID is wrapped into a type since it will be used as key in compile-time maps.
 *
 * @tparam value the constant ID value
 */
template<
	ChannelIDValue value
>
using ChannelID = boost::mpl::integral_c< ChannelIDValue, value >;


#endif /* CHANNELID_HPP_ */
