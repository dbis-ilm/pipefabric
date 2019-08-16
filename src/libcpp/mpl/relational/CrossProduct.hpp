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
 * CrossProduct.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_RELATIONAL_CROSSPRODUCT_HPP_
#define LIBCPP_MPL_RELATIONAL_CROSSPRODUCT_HPP_

#include "JoinTupleWithRelation.hpp"
#include "libcpp/mpl/sequences/SequenceJoiner.hpp"

#include <boost/mpl/transform.hpp>
#include <boost/mpl/iterator_range.hpp>

namespace ns_mpl {

namespace impl {

/**
 * @brief The actual implementation for the a cross product between compile-time relations.
 *
 * The actual implementation case is selected via template specialization through the
 * variadic argument list.
 *
 * @tparam Relations
 *     a list of relations for calculating the cross product
 */
template<
	typename... Relations
>
struct crossProductImpl;


/**
 * @brief Base case for a single relation.
 *
 * The cross product of a single relation is just the relation itself.
 * We wrap all tuples of the relation in an iterator range to generate an error if something
 * else than a sequence is passed as argument.
 *
 * @tparam Relation
 *     the input relation
 */
template<
	typename Relation
>
struct crossProductImpl< Relation > {
	// return all tuples of the relation without changing them
	typedef typename boost::mpl::iterator_range<
		typename boost::mpl::begin< Relation >::type,
		typename boost::mpl::end< Relation >::type
	> type;
};

/**
 * @brief General case for more than one relation.
 *
 * This case is selected when the list of input relations for the cross product contains
 * at least two relations. It calculates the cross product of the right-most relations first
 * and joins each result tuple with all tuples from the left-most one. A single relation
 * is returned as iterator range comprising all result tuples.
 *
 * @tparam LeftRelation
 *      the left-most relation in the argument list
 * @tparam RightRelations
 *      the remaining relations in the argument list
 */
template<
	typename LeftRelation,
	typename... RightRelations
>
struct crossProductImpl< LeftRelation, RightRelations... > {
	// calculate the cross product of all relations to the right first
	typedef typename crossProductImpl< RightRelations... >::type RightCrossProduct;

	// calculate the cross product of the current left-most relation with the already
	// computed right product with joining each tuple from the left with each from the right
	typedef typename boost::mpl::transform<
		LeftRelation, // transform each tuple of the left input relation
		joinTupleWithRelation<
			boost::mpl::_1,   // join the tuple with
			RightCrossProduct // all tuples from the right cross product
		>,
		SequenceJoiner<> // aggregate the result in a single result relation
	>::type CrossProduct;

	// return an iterator range containing all result tuples
	// we need to do this since the calculation is lazy and will fail if more than two
	// relations are passed without enforcing the evaluation to the actual tuples
	// that are referred by the iterators (the meta functions would be combined and joined
	// instead of the underlying tuples forming the result relation)
	typedef boost::mpl::iterator_range<
		typename boost::mpl::begin< CrossProduct >::type,
		typename boost::mpl::end< CrossProduct >::type
	> type;
};


} /* end namespace impl */


/**
 * @brief A meta function for a cross product between compile-time relations.
 *
 * This meta function constructs the cross product between the compile-time relations
 * passed as arguments.The partial results are combined in a single result relation.
 *
 * @tparam Relations
 *     a list of relations for calculating the cross product
 */
template<
	typename... Relations
>
struct crossProduct {
	typedef typename impl::crossProductImpl< Relations... >::type type;
};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_RELATIONAL_CROSSPRODUCT_HPP_ */
