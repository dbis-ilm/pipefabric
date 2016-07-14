/*
 * JoinTuples.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_RELATIONAL_JOINTUPLES_HPP_
#define LIBCPP_MPL_RELATIONAL_JOINTUPLES_HPP_

#include <boost/mpl/joint_view.hpp>


namespace ns_mpl {

/**
 * @brief Join two compile-time tuples.
 *
 * This meta function combines two compile-time tuples into a single one, appending the values
 * of @c Tuple2 to @c Tuple1. It is implemented as a lazy mpl::joint_view.
 *
 * @tparam Tuple1
 *     the first tuple
 * @tparam Tuple2
 *     the second tuple
 */
template<
	typename Tuple1,
	typename Tuple2
>
struct joinTuples :
	public boost::mpl::joint_view<
		Tuple1,
		Tuple2
	>
{};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_JOINTUPLES_HPP_ */
