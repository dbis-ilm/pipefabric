/*
 * Relation.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_RELATIONAL_RELATION_HPP_
#define LIBCPP_MPL_RELATIONAL_RELATION_HPP_

#include "Tuple.hpp"


namespace ns_mpl {

/**
 * @brief A compile-time relation.
 *
 * This class represents a relation for storing multiple compile-time tuples.
 * It is implemented as alias mapping to an mpl::vector.
 *
 * TODO Implement more logic here to distinguish between tuples and relations,
 *      allowing relations only comprising tuples.
 *
 * @tparam Types
 *     the list of compile-time tuples
 */
template< typename... Types >
using Relation = boost::mpl::vector< Types... >;


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_RELATION_HPP_ */
