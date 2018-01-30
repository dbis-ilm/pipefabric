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
 * GenerateInputChannelGroup.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef GENERATEINPUTCHANNELGROUP_HPP_
#define GENERATEINPUTCHANNELGROUP_HPP_

#include "../parameters/InputChannelParameterTraits.hpp"
#include "../parameters/IsInputChannelParameter.hpp"
#include "../InputChannel.hpp"
#include "../ChannelGroup.hpp"

#include <boost/mpl/lambda.hpp>
#include <boost/mpl/assert.hpp>


namespace impl {

/**
 * @brief Meta function creating a new @c InputChannel type with a @c ChannelID
 *        for a given @c Consumer and a set of @c InputChannelParameters.
 *
 * @tparam Consumer
 *           the consumer endpoint that shall be bound to the new channel type
 * @tparam ChannelID
 *           an ID used to uniquely identify the new channel type in the consumer
 * @tparam InputChannelParameters
 *           a structure comprising all parameters describing the new channel,
 *           must satisfy the @c InputChannelParameterTraits
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename Consumer,
	typename ChannelID,
	typename InputChannelParameters
>
class createInputChannelType {
private:

	BOOST_MPL_ASSERT(( isInputChannelParameter< InputChannelParameters > ));

	// extract all relevant parameters for creating the new channel type
	typedef InputChannelParameterTraits< InputChannelParameters > Parameters;
	typedef typename Parameters::ConsumedTypes ConsumedTypes;
	typedef typename Parameters::Slot Slot;

public:

	/// return an input channel type with the requested parameters
	typedef InputChannel< ChannelID, Consumer, ConsumedTypes, Slot > type;
};


/**
 * @brief Meta function for generating a @c ChannelGroup comprising a set of @c InputChannels
 *        for a list of @c InputChannelParameters.
 *
 * @tparam Consumer
 *           the consumer endpoint that shall be bound to the new channel type
 * @tparam InputChannelParameters
 *           a list with all parameters describing the new channel,
 *           all elements must satisfy the @c InputChannelParameterTraits
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename Consumer,
	typename InputChannelParameters
>
using generateInputChannelGroup = channel_group::generateChannelGroup<
	// transform the input channel type generator into a binary meta function class
	typename boost::mpl::lambda<
		createInputChannelType< Consumer, boost::mpl::_1, boost::mpl::_2 >
	>::type,
	// forward the parameter list
	InputChannelParameters
>;


} /* end namespace impl */


#endif /* GENERATEINPUTCHANNELGROUP_HPP_ */
