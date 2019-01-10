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
 * JoinTupleWithRelation.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_RELATIONAL_JOINTUPLEWITHRELATION_HPP_
#define LIBCPP_MPL_RELATIONAL_JOINTUPLEWITHRELATION_HPP_

#include "JoinTuples.hpp"

#include <boost/mpl/transform_view.hpp>


namespace ns_mpl {

/**
 * @brief Meta function that joins a single compile-time tuple with all tuples stored in a relation.
 *
 * This meta function appends all values stored in the compile-time @c Tuple to all tuples
 * stored in the compile-time @c Relation. It is implemented as lazy transform view on the
 * @c Relation which applies the @c joinTuples meta function to each @c Relation tuple and
 * the @c Tuple passed as argument.
 *
 * @tparam Tuple
 *     the tuple to be joined
 * @tparam Relation
 *     the relation whose tuples shall be extended
 */
template<
	typename Tuple,
	typename Relation
>
struct joinTupleWithRelation :
	public boost::mpl::transform_view<
		Relation,
		joinTuples< Tuple, boost::mpl::_ >
	>
{};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_JOINTUPLEWITHRELATION_HPP_ */
