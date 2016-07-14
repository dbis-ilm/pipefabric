/*
 * Append.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_APPEND_HPP_
#define LIBCPP_MPL_SEQUENCES_APPEND_HPP_

#include "ToSequence.hpp"

#include <boost/mpl/copy.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/begin_end.hpp>


namespace ns_mpl {

/**
 * @brief Meta function that appends a type (sequence) to another one.
 *
 * This meta function constructs a new type sequence which comprises the same
 * elements as the original passed via argument @c To and the type(s) passed
 * via @c Type. If @c Type is a sequence, its elements will be appended to
 * @c To. Otherwise, the @c Type itself will be appended.
 *
 * The same sequence type as @c To will be returned. So the result of the meta
 * function depends on this type, i.e., whether duplicates are allowed or not
 * and where the elements will occur in the result sequence. By default they
 * are tried to be inserted at the end of @c To. But this parameter will be
 * ignored e.g., by associative sequences.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Type
 *     the (sequence) type to be appended
 * @tparam To
 *     the target sequence
 */
template<
	typename Type,
	typename To
>
struct Append {
	// convert the source type into a sequence if necessary
	typedef typename ToSequence< Type >::type SourceSequence;

	// append the source type(s)
	typedef typename boost::mpl::copy<
		SourceSequence,
		boost::mpl::inserter<
			To,
			boost::mpl::insert<
				boost::mpl::_1,
				boost::mpl::end< boost::mpl::_1 >,
				boost::mpl::_2
			>
		>
	>::type type;
};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_APPEND_HPP_ */
