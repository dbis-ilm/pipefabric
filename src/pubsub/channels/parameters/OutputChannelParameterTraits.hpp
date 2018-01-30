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
 * OutputChannelParameterTraits.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef OUTPUTCHANNELPARAMETERTRAITS_HPP_
#define OUTPUTCHANNELPARAMETERTRAITS_HPP_

#include "ChannelParameterTraits.hpp"


/**
 * @brief Traits class for parameters which will be used for creating @c OutputChannels.
 *
 * @tparam ChannelParameters
 *           the type whose traits are defined here
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelParameters
>
class OutputChannelParameterTraits :
	public ChannelParameterTraits< ChannelParameters >
{
private:
	typedef ChannelParameterTraits< ChannelParameters > BaseTraits;

	/// the parameter structure whose traits are defined here
	typedef ChannelParameters ChannelParms;

public:
	//////   public types   //////

	/// import the channel types from the base traits
	typedef typename BaseTraits::ChannelTypes ProducedTypes;

	/// the signal type which shall be used for publishing data elements
	/// (bound to the produced types)
	typedef typename ChannelParms::Signal Signal;


	//////   public constants   //////


	//////   public interface   //////
};


#endif /* OUTPUTCHANNELPARAMETERTRAITS_HPP_ */
