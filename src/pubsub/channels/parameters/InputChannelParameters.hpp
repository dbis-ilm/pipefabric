/*
 * InputChannelParameters.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef INPUTCHANNELPARAMETERS_HPP_
#define INPUTCHANNELPARAMETERS_HPP_

#include "ChannelParameters.hpp"
#include "../../signals/SynchronizedSlot.hpp"

#include <boost/mpl/if.hpp>


namespace impl {

/// helper structure for isInputChannelParameter traits
struct InputChannelParameterBase {};

} /* end namespace impl */


/**
 * @brief Base type representing parameters for constructing one input channel.
 *
 * @tparam synchronized
 *    flag indicating if a synchronized input channel shall be created
 * @tparam SlotImpl
 *    the slot implementation for handling incoming data elements
 * @tparam ConsumedTypes
 *    the list with data types which are received through the input channel
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	bool synchronized,
	template<typename...> class SlotImpl,
	typename... ConsumedTypes
>
class InputChannelParameters :
	public impl::InputChannelParameterBase,
	public ChannelParameters< true, ConsumedTypes... >
{
public:

	/// flag indicating if a synchronized input channel was requested
	static const bool isSynchronized = synchronized;

	/// the slot type which shall be used for handling incoming data elements
	/// (bound to the consumed types)
	typedef typename boost::mpl::if_c<
		synchronized,

		// if a synchronized slot was requested, wrap the slot implementation into
		// a SynchronizedSlot decorator
		SynchronizedSlot< SlotImpl< ConsumedTypes... > >,

		// otherwise, just return the slot implementation for the incoming data elements
		SlotImpl< ConsumedTypes... >
	>::type Slot;
};


#endif /* INPUTCHANNELPARAMETERS_HPP_ */
