/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef UnaryTransform_hpp_
#define UnaryTransform_hpp_

#include "qop/BaseOp.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "pubsub/Sink.hpp"
#include "pubsub/Source.hpp"
#include "pubsub/signals/DefaultSlotFunction.hpp"
#include "pubsub/signals/DefaultSourceSignal.hpp"


namespace pfabric {

/**
 * @brief A base class for transforming @c InputStreamElements to @c OutputStreamElements.
 *
 * This class is used as base for all components that consume @c InputStreamElements
 * and produce @c OutputStreamElements.
 * It declares two @c InputChannels and @c OutputChannels:
 *   - a @c InputDataChannel for incoming stream elements, including an outdated flag (ID 0)
 *   - a @c InputPunctuationChannel for stream @c Punctuation tuples (ID 1)
 *   - a @c OutputDataChannel for outgoing stream elements, including an outdated flag (ID 0)
 *   - a @c OutputPunctuationChannel for stream @c Punctuation tuples (ID 1)
 *   .
 *
 * @tparam InputStreamElement
 *    the data stream element type consumed by the transform
 * @tparam OutputStreamElement
 *    the data stream element type produced by the transform
 * @tparam SignalImpl
 *    the signal implementation for publishing produced data elements
 *    (default @c DefaultSourceSignal)
 * @tparam synchronized
 *    flag indicating if channels shall synchronize arriving stream elements internally
 *    (default @c false)
 * @tparam SlotImpl
 *    the slot implementation for handling incoming data elements
 *    (default @c DefaultSlotFunction)
 */
template<
	typename InputStreamElement,
	typename OutputStreamElement,
	bool synchronized = false,
	template< typename... > class SignalImpl = DefaultSourceSignal,
	template<typename...> class SlotImpl = DefaultSlotFunction
>
class UnaryTransform :

	public Sink<   // generate input channels
		InputChannelParameters< // IN ID 0 - data channel
			synchronized, SlotImpl, InputStreamElement, bool
		>,
		InputChannelParameters< // IN ID 1 - punctuation channel
			synchronized, SlotImpl, PunctuationPtr
		>
	>,

	public DataSource<OutputStreamElement, SignalImpl>
{
private:

	//< the base sink type providing the input channels

	typedef Sink<
		InputChannelParameters< synchronized, SlotImpl, InputStreamElement, bool >,
		InputChannelParameters<	synchronized, SlotImpl, PunctuationPtr >
	> SinkBase;
public:

	UnaryTransform( std::string name = "" ) :
		SinkBase( name ), DataSource<OutputStreamElement, SignalImpl> ( name ) {
	}

	//< the common interface for all incoming data stream elements
	typedef StreamElementTraits< InputStreamElement > InputDataElementTraits;


	//< the input channel type for incoming data elements
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 0, InputDataChannel );

	//< the input channel type for incoming punctuation tuples
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 1, InputPunctuationChannel );


	/**
	 * @brief Get a reference to the operator's input data channel.
	 *
	 * @return a reference to the incoming data channel
	 */
	InputDataChannel& getInputDataChannel() {
		return SinkBase::template getInputChannelByID< 0 >();
	}

	/**
	 * @brief Get a reference to the operator's punctuation data channel.
	 *
	 * @return a reference to the punctuation data channel
	 */
	InputPunctuationChannel& getInputPunctuationChannel() {
		return SinkBase::template getInputChannelByID< 1 >();
	}
};

}


#endif /* UnaryTransform_hpp_ */
