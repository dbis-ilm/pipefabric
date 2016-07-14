/*
 * DataSource.hpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#ifndef DATASOURCE_HPP_
#define DATASOURCE_HPP_

#include "qop/BaseOp.hpp"
#include "core/StreamElementTraits.hpp"
#include "pubsub/Source.hpp"
#include "pubsub/signals/DefaultSourceSignal.hpp"


namespace pfabric {

/**
 * @brief A @c Source class for producing data stream elements.
 *
 * This class is used as base for all components that purely produce data stream elements.
 * It declares two @c OutputChannels:
 *   - a @c OutputDataChannel for outgoing stream elements, including an outdated flag (ID 0)
 *   - a @c OutputPunctuationChannel for stream @c Punctuation tuples (ID 1)
 *   .
 *
 * @tparam StreamElement
 *    the data stream element type produced by the source
 * @tparam SignalImpl
 *    the signal implementation for publishing produced data elements
 *    (default @c DefaultSourceSignal)
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename StreamElement,
	template< typename... > class SignalImpl = DefaultSourceSignal
>
class DataSource :
	public BaseOp, // inherit common runtime operator interface
	public Source< // generate output channels
		OutputChannelParameters< // OUT ID 0 - data channel
			SignalImpl, StreamElement, bool
		>,
		OutputChannelParameters< // OUT ID 1 - punctuation channel
			SignalImpl, PunctuationPtr
		>
	>
{
private:

	/// the base source type providing the output channels
	typedef Source< // generate output channels
		OutputChannelParameters< SignalImpl, StreamElement, bool >,
		OutputChannelParameters< SignalImpl, PunctuationPtr	>
	> SourceBase;

public:

	/// inherit constructors
	using SourceBase::SourceBase;


	/// the common interface for all outgoing data stream elements
	typedef StreamElementTraits< StreamElement > OutputDataElementTraits;

	/// the output channel type for outgoing data elements
	IMPORT_OUTPUT_CHANNEL_TYPE( SourceBase, 0, OutputDataChannel );

	/// the output channel type for outgoing punctuation tuples
	IMPORT_OUTPUT_CHANNEL_TYPE( SourceBase, 1, OutputPunctuationChannel );


	/**
	 * @brief Get a reference to the source's data channel.
	 *
	 * @return a reference to the data channel
	 */
	OutputDataChannel& getOutputDataChannel() {
		return SourceBase::template getOutputChannelByID< 0 >();
	}

	/**
	 * @brief Get a reference to the sinks data channel.
	 *
	 * @return a reference to the data channel
	 */
	OutputPunctuationChannel& getOutputPunctuationChannel() {
		return SourceBase::template getOutputChannelByID< 1 >();
	}
};


} /* end namespace pquery */


#endif /* DATASOURCE_HPP_ */
