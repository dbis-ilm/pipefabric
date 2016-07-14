/*
 * ChannelParameters.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef CHANNELPARAMETERS_HPP_
#define CHANNELPARAMETERS_HPP_

#include <boost/mpl/vector.hpp>


/**
 * @brief Common base class for declaring parameters that will be used to create channels.
 *
 * @tparam IsInputChannel
 *     flag indicating if the parameter structure describes an @c InputChannel (if @c true)
 *     or an @c OutputChannel (if @c false)
 * @tparam Types
 *     a list of data types that are passed through the channel to be created
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	bool IsInputChannel,
	typename... Types
>
class ChannelParameters {
public:

	/// the compile-time flag indicating if the channel acts as input our output channel
	static const bool isInputChannel = IsInputChannel;

	/// a compile-time sequence comprising all channel types
	typedef typename boost::mpl::vector< Types... >::type ChannelTypes;

	// TODO static channel name struct
};


#endif /* CHANNELPARAMETERS_HPP_ */
