/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/*
 * StaticForEach.hpp
 *
 *  Created on: Jun 3, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_MPL_ALGORITHMS_STATICFOREACH_HPP_
#define LIBCPP_MPL_ALGORITHMS_STATICFOREACH_HPP_

#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>

namespace ns_mpl {

namespace impl {

/**
 * @brief Specialization when the end of the sequence is not reached.
 *
 * This specialization is instantiated when the current element iterator does
 * not point to the end of the sequence (has a different type than the
 * @c EndIter). The @c Functor is applied to the current element type.
 *
 * @tparam CurrentIter
 *     the iterator to the current element in the sequence
 * @tparam EndIter
 *     the sequence's end iterator
 * @tparam Functor
 *     the functor to be applied on each element in the sequence
 */
template<
	typename CurrentIter,
	typename EndIter,
	typename Functor
>
struct StaticForEachImpl {

	/**
	 * @brief Apply the functor.
	 *
	 * The functor is applied on the current element in the sequence and
	 * recursively on the following elements.
	 */
	static void apply() {
		// extract the current type from the iterator
		typedef typename boost::mpl::deref< CurrentIter >::type CurrentType;

		typedef typename boost::mpl::lambda< Functor >::type F;

		// invoke the functor for that type
		F::template apply< CurrentType > ();

		// the iterator for the following type
		typedef typename boost::mpl::next< CurrentIter >::type Next;

		// apply the functor to the following elements in the sequence
		StaticForEachImpl< Next, EndIter, Functor >::apply();
	}
};


/**
 * @brief Specialization when the end of the sequence is reached.
 *
 * This specialization is instantiated when the current element iterator is the
 * same type as the @c EndIter. Nothing happens here.
 *
 * @tparam EndIter
 *     the type of the sequence's end iterator
 * @tparam Functor
 *     the functor to be applied on each element in the sequence
 */
template<
	typename EndIter,
	typename Functor
>
struct StaticForEachImpl< EndIter, EndIter, Functor > {

	/**
	 * @brief Apply the functor.
	 *
	 * Nothing happens here.
	 */
	static void apply() {}
};

} /* end namespace impl */


/**
 * @brief Apply a functor on each element in a type sequence.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Sequence
 *     the type sequence to be processed
 * @tparam Functor
 *     the meta function class to be applied on each element in the sequence
 */
template<
	typename Sequence,
	typename Functor
>
void staticForEach() {

	/// type iterator for first type in the sequence
	typedef typename boost::mpl::begin< Sequence >::type begin;

	/// type iterator beyond last type in the sequence
	typedef typename boost::mpl::end< Sequence >::type end;

	impl::StaticForEachImpl< begin, end, Functor >::apply();
};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_ALGORITHMS_STATICFOREACH_HPP_ */
