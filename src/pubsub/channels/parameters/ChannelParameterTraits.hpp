/*
 * ChannelParameterTraits.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef CHANNELPARAMETERTRAITS_HPP_
#define CHANNELPARAMETERTRAITS_HPP_


/**
 * @brief Base class for common parameters which will be used for creating channels
 *
 * @tparam ChannelParameters
 *            the ChannelParameter implementation for which the traits are defined
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelParameters
>
class ChannelParameterTraits {
private:

	/// the ChannelParameter component whose traits are defined here
	typedef ChannelParameters ChannelParms;

public:
	//////   public types   //////

	/// a compile-time sequence comprising all channel types
	typedef typename ChannelParms::ChannelTypes ChannelTypes;

	// TODO static channel name struct

	//////   public constants   //////

	/// the compile-time flag indicating if the channel acts as input our output channel
	static const bool isInputChannel = ChannelParms::isInputChannel;


	//////   public interface   //////
};


#endif /* CHANNELPARAMETERTRAITS_HPP_ */
