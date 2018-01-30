/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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
 * Flatten.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_FLATTEN_HPP_
#define LIBCPP_MPL_SEQUENCES_FLATTEN_HPP_

#include "Append.hpp"
#include "Appender.hpp"
#include "ToSequence.hpp"

#include <boost/mpl/is_sequence.hpp>

namespace ns_mpl {

namespace impl {

/**
 * @brief Meta function to flatten a (sequence of)* sequences into a single sequence.
 *
 * This meta function unnests all types in nested sequences of @c Type and transforms
 * them into a single result sequence comprising all types (with duplicates).
 *
 * Two specializations for this meta function exist, one for sequence arguments
 * and one for non-sequences. In the former case, all elements in the sequence
 * are recursively flattened and appended to the final result. In the latter
 * case, a sequence comprising the argument type only is returned.
 *
 * @tparam Type
 *     the type to be flattened
 * @tparam Enable
 *     flag to chose the right implementation
 */
template<
	typename Type,
	class Enable = void
>
struct FlattenImpl;


/**
 * @brief Specialization for non-sequence arguments.
 *
 * This specialization is the base case for the recursion. It returns the element
 * passed as argument as sequence comprising the argument type only. This is
 * required for appending it to previous results in a uniform manner.
 *
 * @tparam NoSequence
 *     a non-sequence type argument
 */
template<
	typename NoSequence
>
struct FlattenImpl<
		NoSequence,
		typename boost::enable_if<
			// do only select this specialization if no sequence is passed
			boost::mpl::not_<
				boost::mpl::is_sequence< NoSequence >
			>
		>::type
	>
{
	typedef typename ToSequence< NoSequence >::type type;
};

/**
 * @brief Specialization for sequence arguments.
 *
 * This specialization is the general case for the recursion when a sequence
 * (of possibly sequences) is passed as argument. Its elements are flattened
 * recursively and appended to a final result type vector.
 *
 * @tparam Sequence
 *     a sequence type argument
 */
template<
	typename Sequence
>
struct FlattenImpl<
		Sequence,
		typename boost::enable_if<
			// do only select this specialization if a sequence is passed
			boost::mpl::is_sequence< Sequence >
		>::type
	>
{
	typedef typename boost::mpl::transform<
		Sequence, // iterate over each element in the sequence
		FlattenImpl< boost::mpl::_1 >, // (recursively) flatten them
		Appender<                // appending
			boost::mpl::_2,      // the currently flattened element(s)
			boost::mpl::_1,      // to the elements collected so far
			boost::mpl::vector<> // starting with an initially empty vector
		>
	>::type type;
};

} /* end namespace impl */


/**
 * @brief Meta function that flattens a (sequence of)* sequences into a single sequence.
 *
 * This meta function examines a @c Type passed as argument and returns a single
 * sequence comprising all types nested in subsequences. Duplicate entries are
 * allowed.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Type
 *     the type to be flattened
 */
template<
	typename Type
>
struct Flatten {
	typedef typename impl::FlattenImpl< Type >::type type;
};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_FLATTEN_HPP_ */
