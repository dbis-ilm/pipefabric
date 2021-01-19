/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/*
 * ChannelParameterTraits.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef CHANNELPARAMETERTRAITS_HPP_
#define CHANNELPARAMETERTRAITS_HPP_


/**
 * @brief Base class for common parameters which will be used for creating channels
 *
 * @tparam ChannelParameters
 *            the ChannelParameter implementation for which the traits are defined
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelParameters
>
class ChannelParameterTraits {
private:

	/// the ChannelParameter component whose traits are defined here
	typedef ChannelParameters ChannelParms;

public:
	//////   public types   //////

	/// a compile-time sequence comprising all channel types
	typedef typename ChannelParms::ChannelTypes ChannelTypes;

	// TODO static channel name struct

	//////   public constants   //////

	/// the compile-time flag indicating if the channel acts as input our output channel
	static const bool isInputChannel = ChannelParms::isInputChannel;


	//////   public interface   //////
};


#endif /* CHANNELPARAMETERTRAITS_HPP_ */
