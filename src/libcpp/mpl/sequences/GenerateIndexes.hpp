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
 * GenerateIndexes.hpp
 *
 *  Created on: Feb 5, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_MPL_SEQUENCES_GENERATEINDEXES_HPP_
#define LIBCPP_MPL_SEQUENCES_GENERATEINDEXES_HPP_

namespace ns_mpl {

/**
 * @brief A compile-time structure comprising a sequence of integer values.
 */
template< int... > struct IndexTuple {};

namespace impl {

/**
 * @brief Generator compile-time function for a consecutive sequence of integers.
 *
 * @tparam CurrentIndex
 *    the value of the current index which is generated
 * @tparam LastIndex
 *    the final index value to be generated (exclusive)
 * @tparam IntTuple
 *    the tuple generated so far
 */
template<
	int CurrentIndex,
	int LastIndex,
	typename IntTuple
>
struct generateIndexesImpl;

/**
 * @brief Base case instantiation where the @c LastIndex has been reached.
 *
 * @tparam LastIndex
 *    the final index value to be generated (exclusive)
 * @tparam IntTuple
 *    the tuple generated so far
 */
template<
	int LastIndex,
	int... Indexes
>
struct generateIndexesImpl<
	LastIndex, LastIndex,    // current index = last index
	IndexTuple< Indexes... > // the accumulated result tuple
>
{
	// just return the result collected so far
	typedef IndexTuple< Indexes... > type;
};

template<
	int CurrentIndex,
	int LastIndex,
	int... Indexes
>
struct generateIndexesImpl<
	CurrentIndex, LastIndex, // different current and last index
	IndexTuple< Indexes... > // the accumulated result tuple
>
{
	// return recursively generated index tuple
	typedef typename generateIndexesImpl<
		CurrentIndex+1, LastIndex,             // continue with the next index
		IndexTuple< Indexes..., CurrentIndex > // append the current index to the current tuple
	>::type type;
};

} /* end namespace impl */


/**
 * @brief Generate a compile-time sequence of integers.
 *
 * This meta function will generate a sequence of integers in the range of
 * [@c FirstIndex, ..., @c FirstIndex + @c NumIndices ).
 *
 * This can be helpful to unroll variadic arguments packs where their position is required,
 * like placeholders, etc.
 *
 * @tparam NumIndices
 *    the total number of indices in the sequence
 * @tparam FirstIndex
 *    the first index number in the generated sequence (inclusive)
 *    (default 0)
 */
template<
	unsigned int NumIndices,
	int FirstIndex = 0
>
struct generateIndexes {
	typedef typename impl::generateIndexesImpl<
		FirstIndex,
		FirstIndex + NumIndices,
		IndexTuple<>
	>::type type;
};

} /* end namespace ns_mpl */

#endif /* LIBCPP_MPL_SEQUENCES_GENERATEINDEXES_HPP_ */
