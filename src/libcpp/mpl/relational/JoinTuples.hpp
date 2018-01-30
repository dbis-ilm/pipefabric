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
 * JoinTuples.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_RELATIONAL_JOINTUPLES_HPP_
#define LIBCPP_MPL_RELATIONAL_JOINTUPLES_HPP_

#include <boost/mpl/joint_view.hpp>


namespace ns_mpl {

/**
 * @brief Join two compile-time tuples.
 *
 * This meta function combines two compile-time tuples into a single one, appending the values
 * of @c Tuple2 to @c Tuple1. It is implemented as a lazy mpl::joint_view.
 *
 * @tparam Tuple1
 *     the first tuple
 * @tparam Tuple2
 *     the second tuple
 */
template<
	typename Tuple1,
	typename Tuple2
>
struct joinTuples :
	public boost::mpl::joint_view<
		Tuple1,
		Tuple2
	>
{};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_JOINTUPLES_HPP_ */
