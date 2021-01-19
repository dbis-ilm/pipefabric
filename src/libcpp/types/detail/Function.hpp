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
 * Function.hpp
 *
 *  Created on: Jan 27, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_FUNCTION_HPP_
#define LIBCPP_TYPES_FUNCTION_HPP_


#ifdef USE_BOOST_FUNCTION
// use boost function

#include <boost/function.hpp>

namespace ns_types {


template<
	typename ReturnType,
	typename... Arguments
>
using Function = boost::function< ReturnType( Arguments... ) >;

} /* end namespace ns_types */

#else
// use standard function

#include <functional>

namespace ns_types {


template<
	typename ReturnType,
	typename... Arguments
>
using Function = std::function< ReturnType( Arguments... ) >;


} /* end namespace ns_types */

#endif


#endif /* LIBCPP_TYPES_FUNCTION_HPP_ */
