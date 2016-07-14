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
