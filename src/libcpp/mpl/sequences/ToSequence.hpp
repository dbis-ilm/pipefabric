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
 * ToSequence.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_TOSEQUENCE_HPP_
#define LIBCPP_MPL_SEQUENCES_TOSEQUENCE_HPP_

#include <boost/utility/enable_if.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/single_view.hpp>


namespace ns_mpl {

namespace impl {

/**
 * @brief A meta function which converts a type into a sequence if necessary.
 *
 * This meta function converts the @c Type argument into a type satisfying the
 * @c boost::mpl sequence concept. If the @c Type is already a sequence, it
 * will be returned unmodified. Otherwise, it will be wrapped into a
 * @c boost::mpl::single_view. The required implementation is selected via
 * the @c Enable parameter.
 *
 * @tparam Type
 *     the type to be converted into an @c mpl::sequence
 * @tparam Enable
 *     flag for choosing the right implementation
 */
template<
	typename Type,
	class Enable = void
>
struct ToSequenceImpl;


/**
 * @brief Specialization for a sequence parameter.
 *
 * This meta function specialization is instantiated when a type is passed as
 * arguement that is already a @c boost::mpl::sequence. It is returned
 * unmodified.
 *
 * @tparam Sequence
 *     a type argument that is already a sequence
 */
template<
	typename Sequence
>
struct ToSequenceImpl<
		Sequence,
		typename boost::enable_if<
			// do only select this specialization if a sequence is passed
			boost::mpl::is_sequence< Sequence >
		>::type
	>
{
	// return the input sequence as is
	typedef Sequence type;
};

/**
 * @brief Specialization for a non-sequence parameter.
 *
 * This meta function specialization is instantiated when a type is passed as
 * arguement that is not a @c boost::mpl::sequence. It is wrapped into a
 * @c boost::mpl::single_view.
 *
 * @tparam NoSequence
 *     a type argument that is not a sequence
 */
template< typename NoSequence >
struct ToSequenceImpl<
		NoSequence,
		typename boost::enable_if<
			// do only select this specialization if no sequence is passed
			boost::mpl::not_<
				boost::mpl::is_sequence< NoSequence >
			>
		>::type
	>
{
	// create a view on a sequence solely comprising the argument type
	typedef boost::mpl::single_view< NoSequence > type;
};


} /* end namespace impl */


/**
 * @brief Meta function which converts a type into a sequence if necessary.
 *
 * This meta function examines the @c Type passed as argument and wraps it into
 * a type that satisfies the @c boost::mpl::sequence requirements if necessary.
 * If a @c Type is passed as argument that is already a sequence, it is returned
 * unmodified.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Type
 *     the type to be transformed into a @c boost::mpl::sequence
 */
template<
	typename Type
>
struct ToSequence {
	typedef typename impl::ToSequenceImpl< Type >::type type;
};


} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_TOSEQUENCE_HPP_ */
