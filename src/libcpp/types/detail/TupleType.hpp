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
 * TupleType.hpp
 *
 *  Created on: Jun 5, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TUPLETYPE_HPP_
#define LIBCPP_TYPES_TUPLETYPE_HPP_

#include "libcpp/utilities/PrintCSV.hpp"

#include <tuple>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/variant.hpp>


namespace ns_types {

/**
 * @brief A tuple of heterogeneous types.
 */
template< typename... TupleElements >
using TupleType = std::tuple< TupleElements... >;

template< typename Tuple >
using TupleSize = std::tuple_size< Tuple >;

template< std::size_t ID, typename Tuple >
using TupleElement = std::tuple_element< ID, Tuple >;

template< std::size_t I, typename... Args >
auto get( Args&&... args ) -> decltype( std::get<I>( std::forward< Args >(args)... ) ) {
	return std::get<I>( std::forward< Args >(args)... );
}


namespace impl {

/**
 * \brief A helper template for dynamically access the components of a tuple.
 *
 * dynamic_get provides a way for accessing components (fields) of a tuple at runtime.
 * Whereas for std::get<> the index has to be known at compile time, dynamic_get allows
 * to provide the index at runtime:
 *    int idx = 1;
 *    boost::variant& val = dynamic_get(idx, *tup);
 *
 * Note, that dynamic_get returns a boost::variant object.
 *
 * Taken from http://stackoverflow.com/questions/8194227/how-to-get-the-i-th-element-from-an-stdtuple-when-i-isnt-know-at-compile-time
 */
template< size_t n, typename... Types >
boost::variant<Types...> dynamic_get_impl(size_t i, const TupleType<Types...>& tpl) {
	if (i == n)
		return std::get<n>(tpl);
	else if (n == sizeof...(Types) - 1)
		throw std::out_of_range("Tuple element out of range.");
	else
		return dynamic_get_impl<(n < sizeof...(Types)-1 ? n+1 : 0)>(i, tpl);
}

} /* end namespace impl */


template <typename... Types>
boost::variant<Types...> dynamic_get(size_t i, const TupleType<Types...>& tpl) {
	return impl::dynamic_get_impl<0>(i, tpl);
}

} /* end namespace ns_types */


template< typename... TupleElements >
std::ostream& operator<< ( std::ostream& out, const ns_types::TupleType< TupleElements... >& tuple ) {
	// create a default CSV printer for the entire tuple
	typedef ns_utilities::PrintCSV< ns_types::TupleSize< decltype(tuple) >::value > printAsCSV;
	boost::fusion::for_each( tuple, printAsCSV( out, "(", ")" ) );
	return out;
}


#endif /* LIBCPP_TYPES_TUPLETYPE_HPP_ */
