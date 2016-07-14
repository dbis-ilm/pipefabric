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
