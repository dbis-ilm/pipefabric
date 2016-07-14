/*
 * GenerateChannelConsumerInterface.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef GENERATECHANNELCONSUMERINTERFACE_HPP_
#define GENERATECHANNELCONSUMERINTERFACE_HPP_

#include "ChannelConsumer.hpp"
#include "../ChannelGroup.hpp"

#include <boost/mpl/inherit.hpp>
#include <boost/mpl/inherit_linearly.hpp>


namespace impl {

/**
 * @brief Meta function for generating a @c ChannelConsumer interface for each channel in
 *        an @c InputChannelGroup.
 *
 * This meta function uses recursive inheritance to add a @c ChannelConsumer interface
 * for each channel type in the @c InputChannelGroup passed as argument.
 * The type which shall have the generated interfaces must inherit from this type.
 *
 * @tparam InputChannelGroup
 *           a @c ChannelGroup comprising all @c InputChannel types for which a
 *           @c ChannelConsumer interface shall be generated
 */
template<
	typename InputChannelGroup
>
using generateChannelConsumerInterface = typename boost::mpl::inherit_linearly<
	// for all channels in the group
	typename channel_group::getChannels< InputChannelGroup >::type,
	boost::mpl::inherit<                   // inherit
		boost::mpl::_1,                    // from the previous type
		ChannelConsumer< boost::mpl::_2 >  // adding another ChannelConsumer interface
	>
>::type;

} /* end namespace impl */


#endif /* GENERATECHANNELCONSUMERINTERFACE_HPP_ */
