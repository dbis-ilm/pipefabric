/*
 * HasDuplicates.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_HASDUPLICATES_HPP_
#define LIBCPP_MPL_SEQUENCES_HASDUPLICATES_HPP_

#include "RemoveDuplicates.hpp"

#include <boost/mpl/size.hpp>
#include <boost/mpl/less.hpp>

namespace ns_mpl {

/**
 * @brief Meta function checking if a sequence contains duplicate entries.
 *
 * This meta function returns a boolean constant, @c true_ in case duplicate
 * entries are contained in the argument @c Sequence, @c false_ otherwise.
 *
 * TODO This implementation is not very efficient since a new type sequence
 *      without duplicates is constructed first and its size is compared to
 *      the source sequence. There might be something more sophisticated.
 *      Further, for certain sequence types, duplicates are not possible.
 *      Separate specializations could be created for them.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Sequence
 *     the sequence to be checked
 */
template< typename Sequence >
struct HasDuplicates {
	typedef typename boost::mpl::less<
		// construct a new sequence without duplicates
		boost::mpl::size< typename RemoveDuplicates< Sequence >::type >,
		// and compare its size to the size of the input sequence
		boost::mpl::size< Sequence >
	>::type type;

	// the nested compile-time constant return value
	static const bool value = type::value;
};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_HASDUPLICATES_HPP_ */
