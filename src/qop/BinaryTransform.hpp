/*
 * BinaryTransform.hpp
 *
 *  Created on: Feb 20, 2015
 *      Author: fbeier
 */

#ifndef BINARYTRANSFORM_HPP_
#define BINARYTRANSFORM_HPP_

#include "qop/BaseOp.hpp"
#include "pubsub/Sink.hpp"
#include "pubsub/Source.hpp"
#include "pubsub/signals/DefaultSlotFunction.hpp"
#include "pubsub/signals/DefaultSourceSignal.hpp"


namespace pfabric {


/**
 * @brief A base class for transforming elements from two sources to @c OutputStreamElements.
 *
 * This class is used as base for all components that consume @c LeftInputStreamElements from
 * one (left) source and @c RightInputStreamElements from another (right) one and produce
 * @c OutputStreamElements.
 * It declares three @c InputChannels and one @c OutputChannel:
 *   - a @c LeftInputDataChannel for incoming stream elements from the left source,
 *          including an outdated flag (ID 0)
 *   - a @c RightInputDataChannel for incoming stream elements from the right source,
 *          including an outdated flag (ID 1)
 *   - a @c InputPunctuationChannel for stream @c Punctuation tuples (ID 2)
 *   - a @c OutputDataChannel for outgoing stream elements, including an outdated flag (ID 0)
 *   - a @c OutputPunctuationChannel for stream @c Punctuation tuples (ID 1)
 *   .
 *
 * @tparam LeftInputStreamElement
 *    the data stream element type consumed by the transform from its left source
 * @tparam RightInputStreamElement
 *    the data stream element type consumed by the transform from its right source
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
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename LeftInputStreamElement,
	typename RightInputStreamElement,
	typename OutputStreamElement,
	template< typename... > class SignalImpl = DefaultSourceSignal,
	bool synchronized = false,
	template<typename...> class SlotImpl = DefaultSlotFunction
>
class BinaryTransform :
	// public BaseOp, // inherit common runtime operator interface
	public Sink<   // generate input channels
		InputChannelParameters< // IN ID 0 - left data channel
			synchronized, SlotImpl, LeftInputStreamElement, bool
		>,
		InputChannelParameters< // IN ID 1 - right data channel
			synchronized, SlotImpl, RightInputStreamElement, bool
		>,
		InputChannelParameters< // IN ID 2 - punctuation channel
			synchronized, SlotImpl, PunctuationPtr
		>
	>,
	public DataSource<OutputStreamElement, SignalImpl>
	/*
	public Source< // generate output channels
		OutputChannelParameters< // OUT ID 0 - data channel
			SignalImpl, OutputStreamElement, bool
		>,
		OutputChannelParameters< // OUT ID 1 - punctuation channel
			SignalImpl, PunctuationPtr
		>
	>
	*/
{
private:

	/// the base sink type providing the input channels
	typedef Sink<
		InputChannelParameters< synchronized, SlotImpl, LeftInputStreamElement, bool >,
		InputChannelParameters< synchronized, SlotImpl, RightInputStreamElement, bool >,
		InputChannelParameters<	synchronized, SlotImpl, PunctuationPtr >
	> SinkBase;

	/// the base source type providing the output channels
	/*
	typedef Source< // generate output channels
		OutputChannelParameters< SignalImpl, OutputStreamElement, bool >,
		OutputChannelParameters< SignalImpl, PunctuationPtr	>
	> SourceBase;
*/
public:

	BinaryTransform( std::string name = "" ) :
		SinkBase( name ), DataSource<OutputStreamElement, SignalImpl> ( name ) {
	}


	/// the common interface for all incoming data stream elements from the left source
	typedef StreamElementTraits< LeftInputStreamElement > LeftInputDataElementTraits;

	/// the common interface for all incoming data stream elements from the right source
	typedef StreamElementTraits< RightInputStreamElement > RightInputDataElementTraits;

	/// the common interface for all outgoing data stream elements
	typedef StreamElementTraits< OutputStreamElement > OutputDataElementTraits;

	/// the input channel type for incoming data elements from the left source
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 0, LeftInputDataChannel );

	/// the input channel type for incoming data elements from the right source
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 1, RightInputDataChannel );

	/// the input channel type for incoming punctuation tuples
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 2, InputPunctuationChannel );

	/// the output channel type for outgoing data elements
	// IMPORT_OUTPUT_CHANNEL_TYPE( SourceBase, 0, OutputDataChannel );

	/// the output channel type for outgoing punctuation tuples
	// IMPORT_OUTPUT_CHANNEL_TYPE( SourceBase, 1, OutputPunctuationChannel );


	/**
	 * @brief Get a reference to the operator's input data channel from the left source.
	 *
	 * @return a reference to the incoming data channel from the left source
	 */
	LeftInputDataChannel& getLeftInputDataChannel() {
		return SinkBase::template getInputChannelByID< 0 >();
	}

	/**
	 * @brief Get a reference to the operator's input data channel from the right source.
	 *
	 * @return a reference to the incoming data channel from the right source
	 */
	RightInputDataChannel& getRightInputDataChannel() {
		return SinkBase::template getInputChannelByID< 1 >();
	}

	/**
	 * @brief Get a reference to the operator's punctuation data channel.
	 *
	 * @return a reference to the punctuation data channel
	 */
	InputPunctuationChannel& getInputPunctuationChannel() {
		return SinkBase::template getInputChannelByID< 2 >();
	}

	/**
	 * @brief Get a reference to the operator's outgoing data channel.
	 *
	 * @return a reference to the outgoing data channel
	 */
	 /*
	OutputDataChannel& getOutputDataChannel() {
		return SourceBase::template getOutputChannelByID< 0 >();
	}
*/
	/**
	 * @brief Get a reference to the operator's outgoing punctuation channel.
	 *
	 * @return a reference to the outgoing punctuation channel
	 */
	 /*
	OutputPunctuationChannel& getOutputPunctuationChannel() {
		return SourceBase::template getOutputChannelByID< 1 >();
	}
	*/
};

} /* end namespace pquery */


#endif /* BINARYTRANSFORM_HPP_ */
