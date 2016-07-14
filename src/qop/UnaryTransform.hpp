/*
 * UnaryTransform.hpp
 *
 *  Created on: Feb 15, 2015
 *      Author: fbeier
 */

#ifndef UNARYTRANSFORM_HPP_
#define UNARYTRANSFORM_HPP_

#include "qop/BaseOp.hpp"
#include "qop/DataSource.hpp"
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
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename InputStreamElement,
	typename OutputStreamElement,
	template< typename... > class SignalImpl = DefaultSourceSignal,
	bool synchronized = false,
	template<typename...> class SlotImpl = DefaultSlotFunction
>
class UnaryTransform :
	// public BaseOp, // inherit common runtime operator interface
	public Sink<   // generate input channels
		InputChannelParameters< // IN ID 0 - data channel
			synchronized, SlotImpl, InputStreamElement, bool
		>,
		InputChannelParameters< // IN ID 1 - punctuation channel
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
		InputChannelParameters< synchronized, SlotImpl, InputStreamElement, bool >,
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

	UnaryTransform( std::string name = "" ) :
		SinkBase( name ), DataSource<OutputStreamElement, SignalImpl> ( name ) {
	}


	/// the common interface for all incoming data stream elements
	typedef StreamElementTraits< InputStreamElement > InputDataElementTraits;

	/// the common interface for all outgoing data stream elements
	// typedef StreamElementTraits< OutputStreamElement > OutputDataElementTraits;

	/// the input channel type for incoming data elements
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 0, InputDataChannel );

	/// the input channel type for incoming punctuation tuples
	IMPORT_INPUT_CHANNEL_TYPE( SinkBase, 1, InputPunctuationChannel );

	/// the output channel type for outgoing data elements
	//IMPORT_OUTPUT_CHANNEL_TYPE( SourceBase, 0, OutputDataChannel );

	/// the output channel type for outgoing punctuation tuples
	//IMPORT_OUTPUT_CHANNEL_TYPE( SourceBase, 1, OutputPunctuationChannel );


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


#endif /* UNARYTRANSFORM_HPP_ */
