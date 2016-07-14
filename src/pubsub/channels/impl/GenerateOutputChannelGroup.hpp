/*
 * GenerateOutputChannelGroup.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef GENERATEOUTPUTCHANNELGROUP_HPP_
#define GENERATEOUTPUTCHANNELGROUP_HPP_

#include "../parameters/OutputChannelParameterTraits.hpp"
#include "../parameters/IsOutputChannelParameter.hpp"
#include "../OutputChannel.hpp"
#include "../ChannelGroup.hpp"

#include <boost/mpl/lambda.hpp>


namespace impl {

/**
 * @brief Meta function creating a new @c OutputChannel type with a @c ChannelID
 *        for a given @c Producer and a set of @c OutputChannelParameters.
 *
 * @tparam Producer
 *           the producer endpoint that shall be bound to the new channel type
 * @tparam ChannelID
 *           an ID used to uniquely identify the new channel type in the producer
 * @tparam OutputChannelParameters
 *           a structure comprising all parameters describing the new channel,
 *           must satisfy the @c OutputChannelParameterTraits
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename Producer,
	typename ChannelID,
	typename OutputChannelParameters
>
class createOutputChannelType {
private:

	BOOST_MPL_ASSERT(( isOutputChannelParameter< OutputChannelParameters > ));

	// extract all relevant parameters for creating the new channel type
	typedef OutputChannelParameterTraits< OutputChannelParameters > Parameters;
	typedef typename Parameters::ProducedTypes ProducedTypes;
	typedef typename Parameters::Signal Signal;

public:

	/// return an input channel type with the requested parameters
	typedef OutputChannel< ChannelID, Producer, ProducedTypes, Signal > type;
};


/**
 * @brief Meta function for generating a @c ChannelGroup comprising a set of @c OutputChannels
 *        for a list of @c OutputChannelParameters.
 *
 * @tparam Producer
 *           the producer endpoint that shall be bound to the new channel type
 * @tparam OutputChannelParameters
 *           a structure comprising all parameters describing the new channel,
 *           all elements must satisfy the @c OutputChannelParameterTraits
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename Producer,
	typename OutputChannelParameters
>
using generateOutputChannelGroup = channel_group::generateChannelGroup<
	// transform the input channel type generator into a binary meta function class
	typename boost::mpl::lambda<
		createOutputChannelType< Producer, boost::mpl::_1, boost::mpl::_2 >
	>::type,
	// forward the parameter list
	OutputChannelParameters
>;


} /* end namespace impl */

#endif /* GENERATEOUTPUTCHANNELGROUP_HPP_ */
