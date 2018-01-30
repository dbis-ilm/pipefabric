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
