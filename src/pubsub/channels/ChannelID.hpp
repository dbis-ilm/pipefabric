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
 * ChannelID.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef CHANNELID_HPP_
#define CHANNELID_HPP_

#include <boost/mpl/integral_c.hpp>

/// type for storing a unique channel ID
typedef unsigned int ChannelIDValue;

/**
 * @brief Compile-time constant for storing a unique ChannelID.
 *
 * The ID is wrapped into a type since it will be used as key in compile-time maps.
 *
 * @tparam value the constant ID value
 */
template<
	ChannelIDValue value
>
using ChannelID = boost::mpl::integral_c< ChannelIDValue, value >;


#endif /* CHANNELID_HPP_ */
