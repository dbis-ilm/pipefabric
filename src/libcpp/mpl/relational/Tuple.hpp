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
 * Tuple.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_RELATIONAL_MPL_TUPLE_HPP_
#define LIBCPP_MPL_RELATIONAL_MPL_TUPLE_HPP_

#include <boost/mpl/vector.hpp>


namespace ns_mpl {

/**
 * @brief A compile-time tuple.
 *
 * This class represents a tuple for storing multiple compile-time values.
 * It is implemented as alias mapping to an mpl::vector.
 *
 * TODO Implement more logic here to distinguish between tuples and relations.
 *
 * @tparam Types
 *     the list of compile-time values/types stored in the tuple
 */
template< typename... Types >
using Tuple = boost::mpl::vector< Types... >;


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_MPL_TUPLE_HPP_ */
