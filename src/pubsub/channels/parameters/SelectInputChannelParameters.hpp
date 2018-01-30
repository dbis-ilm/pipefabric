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
 * SelectInputChannelParameters.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef SELECTINPUTCHANNELPARAMETERS_HPP_
#define SELECTINPUTCHANNELPARAMETERS_HPP_

#include "IsInputChannelParameter.hpp"

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/quote.hpp>

/**
 * @brief Meta function selecting all @c InputChannelParameters from a type list.
 *
 * @tparam ChannelParameters
 *    the list of type parameters to be filtered
 */
template<
	typename... ChannelParameters
>
struct SelectInputChannelParameters :
	public boost::mpl::filter_view<
		boost::mpl::vector< ChannelParameters... >,
		boost::mpl::quote1< isInputChannelParameter >
	>::type
{};


#endif /* SELECTINPUTCHANNELPARAMETERS_HPP_ */
