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
 * MakeRelation.hpp
 *
 *  Created on: Jan 5, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_MPL_RELATIONAL_MAKERELATION_HPP_
#define LIBCPP_MPL_RELATIONAL_MAKERELATION_HPP_

#include "Relation.hpp"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/single_view.hpp>


namespace ns_mpl {

/**
 * @brief Construct a compile-time relation as single column of types.
 *
 * This meta function converts a list of types into a relation having a tuple for each type.
 *
 * @tparam RowTypes
 *     the list of types to be converted in compile-time 1-tuples as rows of the relation
 */
template< typename... RowTypes >
struct makeRelation :
	public boost::mpl::transform<
		boost::mpl::vector< RowTypes... >,
		boost::mpl::single_view< boost::mpl::_1 >
	>
{};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_MAKERELATION_HPP_ */
