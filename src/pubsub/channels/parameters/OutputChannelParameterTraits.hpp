/*
 * OutputChannelParameterTraits.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef OUTPUTCHANNELPARAMETERTRAITS_HPP_
#define OUTPUTCHANNELPARAMETERTRAITS_HPP_

#include "ChannelParameterTraits.hpp"


/**
 * @brief Traits class for parameters which will be used for creating @c OutputChannels.
 *
 * @tparam ChannelParameters
 *           the type whose traits are defined here
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelParameters
>
class OutputChannelParameterTraits :
	public ChannelParameterTraits< ChannelParameters >
{
private:
	typedef ChannelParameterTraits< ChannelParameters > BaseTraits;

	/// the parameter structure whose traits are defined here
	typedef ChannelParameters ChannelParms;

public:
	//////   public types   //////

	/// import the channel types from the base traits
	typedef typename BaseTraits::ChannelTypes ProducedTypes;

	/// the signal type which shall be used for publishing data elements
	/// (bound to the produced types)
	typedef typename ChannelParms::Signal Signal;


	//////   public constants   //////


	//////   public interface   //////
};


#endif /* OUTPUTCHANNELPARAMETERTRAITS_HPP_ */
