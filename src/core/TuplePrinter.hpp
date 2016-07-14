/*
 * TuplePrinter.hpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#ifndef TUPLEPRINTER_HPP_
#define TUPLEPRINTER_HPP_

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


} /* end namespace pquery */


#endif /* TUPLEPRINTER_HPP_ */
