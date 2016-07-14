/*
 * InputChannelParameterTraits.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef INPUTCHANNELPARAMETERTRAITS_HPP_
#define INPUTCHANNELPARAMETERTRAITS_HPP_

#include "ChannelParameterTraits.hpp"


/**
 * @brief Traits class for parameters which will be used for creating @c InputChannels.
 *
 * @tparam ChannelParameters
 *           the type whose traits are defined here
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelParameters
>
class InputChannelParameterTraits :
	public ChannelParameterTraits< ChannelParameters >
{
private:
	typedef ChannelParameterTraits< ChannelParameters > BaseTraits;

	/// the parameter structure whose traits are defined here
	typedef ChannelParameters ChannelParms;

public:
	//////   public types   //////

	/// import the channel types from the base traits
	typedef typename BaseTraits::ChannelTypes ConsumedTypes;

	/// the slot type which shall be used for handling incoming data elements
	/// (bound to the consumed types)
	typedef typename ChannelParms::Slot Slot;


	//////   public constants   //////


	//////   public interface   //////
};


#endif /* INPUTCHANNELPARAMETERTRAITS_HPP_ */
