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
 * IsInputChannelParameter.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef ISINPUTCHANNELPARAMETER_HPP_
#define ISINPUTCHANNELPARAMETER_HPP_

#include "InputChannelParameters.hpp"

#include <boost/mpl/bool.hpp>
#include <type_traits>

#include "../../Sink.hpp"

/**
 * @brief Traits class to determine if a given argument describes @c InputChannelParameters.
 *
 * General case, evaluating to @c false_.
 *
 * @tparam ChannelParameter
 *    the type to be checked
 */
template<
	typename ChannelParameter
>
struct isInputChannelParameter :
	public std::is_base_of< impl::InputChannelParameterBase, ChannelParameter >
{};

#endif /* ISINPUTCHANNELPARAMETER_HPP_ */
