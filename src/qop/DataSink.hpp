/*
 * DataSink.hpp
 *
 *  Created on: Feb 15, 2015
 *      Author: fbeier
 */

#ifndef DATASINK_HPP_
#define DATASINK_HPP_

#include "qop/BaseOp.hpp"
#include "pubsub/Sink.hpp"
#include "pubsub/signals/DefaultSlotFunction.hpp"
#include "core/StreamElementTraits.hpp"

namespace pfabric {

/**
 * @brief A @c Sink class for consuming data stream elements.
 *
 * This class is used as base for all components that purely consume data stream elements.
 * It declares two @c InputChannels:
 *   - a @c InputDataChannel for incoming stream elements, including an outdated flag (ID 0)
 *   - a @c InputPunctuationChannel for stream @c Punctuation tuples (ID 1)
 *   .
 *
 * The @c synchronized flag indicates if the input channels internally synchronize
 * concurrently published stream elements arriving at the @c SAME channel.
 * It does @c NOT handle synchronization @c BETWEEN the two channels.
 *
 * @tparam StreamElement
 *    the data stream element type consumed by the sink
 * @tparam synchronized
 *    flag indicating if channels shall synchronize arriving stream elements internally
 *    (default @c false)
 * @tparam SlotImpl
 *    the slot implementation for handling incoming data elements
 *    (default @c DefaultSlotFunction)
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename StreamElement,
	bool synchronized = false,
	template<typename...> class SlotImpl = DefaultSlotFunction
>
class DataSink :
	public BaseOp, // inherit common runtime operator interface
	public Sink<   // generate input channels
		InputChannelParameters< // IN ID 0 - data channel
			synchronized, SlotImpl, StreamElement, bool
		>,
		InputChannelParameters< // IN ID 1 - punctuation channel
			synchronized, SlotImpl, PunctuationPtr
		>
	>
{
private:

	/// the base sink type providing the input channels
	typedef Sink<
		InputChannelParameters< synchronized, SlotImpl, StreamElement, bool >,
		InputChannelParameters<	synchronized, SlotImpl, PunctuationPtr >
	> SinkBase;


public:

	/// inherit constructors
	using SinkBase::SinkBase;


	/// the common interface for all incoming data stream elements
	typedef StreamElementTraits< StreamElement > InputDataElementTraits;

	/// the input channel type for incoming data elements
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 0, InputDataChannel );

	/// the input channel type for incoming punctuation tuples
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 1, InputPunctuationChannel );


	/**
	 * @brief Get a reference to the sinks data channel.
	 *
	 * @return a reference to the data channel
	 */
	InputDataChannel& getInputDataChannel() {
		return SinkBase::template getInputChannelByID< 0 >();
	}

	/**
	 * @brief Get a reference to the sinks data channel.
	 *
	 * @return a reference to the data channel
	 */
	InputPunctuationChannel& getInputPunctuationChannel() {
		return SinkBase::template getInputChannelByID< 1 >();
	}

};


/**
 * @brief A more verbose alias for a synchronized @c DataSink.
 *
 * @see @c DataSink
 *
 * @tparam StreamElement
 *    the data stream element type consumed by the sink
 * @tparam SlotImpl
 *    the slot implementation for handling incoming data elements
 *    (default @c DefaultSlotFunction)
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename StreamElement,
	template<typename...> class SlotImpl = DefaultSlotFunction
>
using SynchronizedDataSink = DataSink< StreamElement, true, SlotImpl >;


} /* end namespace pquery */


#endif /* DATASINK_HPP_ */
