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
 * Appender.hpp
 *
 *  Created on: Jul 8, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_APPENDER_HPP_
#define LIBCPP_MPL_SEQUENCES_APPENDER_HPP_

#include "Append.hpp"

#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/push_back.hpp>

namespace ns_mpl {

/**
 * @brief An inserter for appending types/sequences to another one.
 *
 * This type implements an inserter which can e.g., used in mpl algorithms to
 * extend sequences with other sequence/non-sequence types.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Element
 *     the current element to be inserted
 * @tparam State
 *     the current state of the inserter with the elements collected so far
 * @tparam InitialState
 *     the initial state when the inserter is instantiated
 */
template<
	typename Element,
	typename State,
	typename InitialState
>
struct Appender :
		boost::mpl::inserter<  // is an inserter
			InitialState,      // which extends the initial sequence
			Append<            // with appending
				Element,       // the current element
				State          // to the state processed so far
			>
		>
{};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_APPENDER_HPP_ */
