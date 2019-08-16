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
 * IndexOf.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_INDEXOF_HPP_
#define LIBCPP_MPL_SEQUENCES_INDEXOF_HPP_

#include <boost/mpl/find.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/int.hpp>


namespace ns_mpl {

/**
 * @brief Meta function that returns an index of a type within a sequence.
 *
 * This meta function returns a compile-time integral constant with the index
 * of a @c Type within a @c Sequence. If the @c Type cannot be found in the
 * @c Sequence, @c boos::mpl::size< Sequence > is returned.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Type
 *     the type whose position index shall be determined
 * @tparam Sequence
 *     the sequence which shall be examined
 */
template<
	typename Type,
	typename Sequence
>
struct IndexOf {
	// iterator to the beginning of the input sequence
	typedef typename boost::mpl::begin< Sequence >::type First;

	// search the type entry in the sequence
	typedef typename boost::mpl::find<
		Sequence,
		Type
	>::type Position;

	// calculate the distance between the position relative to the beginning
	// as result type
	typedef typename boost::mpl::distance< First, Position >::type type;

	// nested compile-time constant value
	static const int value = type::value;
};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_INDEXOF_HPP_ */
