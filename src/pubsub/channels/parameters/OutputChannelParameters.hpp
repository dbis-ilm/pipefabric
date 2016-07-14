/*
 * OutputChannelParameters.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef OUTPUTCHANNELPARAMETERS_HPP_
#define OUTPUTCHANNELPARAMETERS_HPP_

#include "ChannelParameters.hpp"


namespace impl {
	/// helper structure for isOutputChannelParameter traits
	struct OutputChannelParameterBase {};
} /* end namespace impl */


/**
 * @brief Base type representing parameters for constructing one output channel.
 *
 * @tparam SignalImpl
 *    the signal implementation for publishing produced data elements
 * @tparam ProducedTypes
 *    the list with data types which are published through the output channel
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	template< typename... > class SignalImpl,
	typename... ProducedTypes
>
class OutputChannelParameters :
	public impl::OutputChannelParameterBase,
	public ChannelParameters< false, ProducedTypes... >
{
public:

	/// the signal type which shall be used for publishing produced data elements
	/// (bound to the produced types)
	typedef SignalImpl< ProducedTypes... > Signal;
};


#endif /* OUTPUTCHANNELPARAMETERS_HPP_ */
