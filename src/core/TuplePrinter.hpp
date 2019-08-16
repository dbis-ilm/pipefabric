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

#ifndef TuplePrinter_hpp_
#define TuplePrinter_hpp_

#include "PFabricTypes.hpp"
#include <ostream>

namespace pfabric {

namespace impl {

/**
 * \brief TuplePrinter is a helper function to print a tuple of any size.
 *
 * TuplePrinter is a helper function to print a Tuple instance of any size and
 * member types to std::ostream. This template should not be directly used, but
 * only via the Tuple members.
 *
 * @tparam Tuple
 *    the tuple type
 * @tparam CurrentIndex
 *    the index of the attribute value to be printed
 */
template<class Tuple, std::size_t CurrentIndex>
struct TuplePrinter;

/**
 * @brief General overload for printing more than 1 element.
 *
 * This specialization will print the remaining elements first and appends the
 * current one after a comma.
 *
 * @tparam Tuple
 *    the tuple type
 * @tparam CurrentIndex
 *    the index of the attribute value to be printed
 */
template<class Tuple, std::size_t CurrentIndex>
struct TuplePrinter {
	static void print( std::ostream& os, const Tuple& t )  {
		TuplePrinter<Tuple, CurrentIndex-1>::print(os, t);
		os << "," << ns_types::get<CurrentIndex-1>(t);
    }
};

/**
 * @brief Specialization for printing a tuple with 1 element.
 *
 * This specialization will just print the element.
 *
 * @tparam Tuple
 *    the tuple type having one element
 */
template<class Tuple>
struct TuplePrinter<Tuple, 1> {
	static void print( std::ostream& os, const Tuple& t ) {
		os << ns_types::get<0>(t);
    }
};

/**
 * @brief Specialization for printing a tuple with no elements.
 *
 * This specialization will do nothing.
 *
 * @tparam Tuple
 *    the tuple type having no elements
 */
template<class Tuple>
struct TuplePrinter<Tuple, 0> {
	static void print( std::ostream& os, const Tuple& t ) {}
};

} /* end namespace impl */


template<class... Args>
void print( std::ostream& os, const ns_types::TupleType<Args...>& t ) {
//	os << "(";
	impl::TuplePrinter<decltype(t), sizeof...(Args)>::print(os, t);
//	os << ")";
}


} /* end namespace pfabric */


#endif /* TuplePrinter_hpp_ */
