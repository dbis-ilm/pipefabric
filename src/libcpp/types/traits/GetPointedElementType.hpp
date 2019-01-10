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
 * GetPointedElementType.hpp
 *
 *  Created on: Feb 16, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TRAITS_GETPOINTEDELEMENTTYPE_HPP_
#define LIBCPP_TYPES_TRAITS_GETPOINTEDELEMENTTYPE_HPP_


namespace ns_types {

/**
 * @brief Meta function to extract the pointed-to element type of a complex pointer class.
 *
 * @tparam Pointer
 *    the pointer class (must provide a dereferencing operator)
 */
template< typename Pointer >
struct getPointedElementType
{
	typedef typename std::remove_cv<
		typename std::remove_reference< Pointer >::type
	>::type _P;

	typedef typename std::remove_reference<
		typename std::result_of<
			decltype( &_P::operator* ) ( _P* )
		>::type
	>::type type;
};

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_TRAITS_GETPOINTEDELEMENTTYPE_HPP_ */
