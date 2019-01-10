/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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
