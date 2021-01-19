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
 * SelectOutputChannelParameters.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef SELECTOUTPUTCHANNELPARAMETERS_HPP_
#define SELECTOUTPUTCHANNELPARAMETERS_HPP_

#include "IsOutputChannelParameter.hpp"

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/vector.hpp>


/**
 * @brief Meta function selecting all @c OutputChannelParameters from a type list.
 *
 * @tparam ChannelParameters
 *    the list of type parameters to be filtered
 */
template<
	typename... ChannelParameters
>
struct SelectOutputChannelParameters :
	public boost::mpl::filter_view<
		boost::mpl::vector< ChannelParameters... >,
		isOutputChannelParameter< boost::mpl::_1 >
	>
{};


#endif /* SELECTOUTPUTCHANNELPARAMETERS_HPP_ */
