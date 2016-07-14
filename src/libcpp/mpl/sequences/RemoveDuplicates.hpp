/*
 * RemoveDuplicates.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_REMOVEDUPLICATES_HPP_
#define LIBCPP_MPL_SEQUENCES_REMOVEDUPLICATES_HPP_

#include <boost/mpl/insert.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/assert.hpp>

namespace ns_mpl {

/**
 * @brief Meta function that removes all duplicate entries from a type sequence.
 *
 * This meta function constructs a new @c boost::mpl::set of types comprising all
 * elements of the source @c Sequence. Thus, all duplicate entries will be removed.
 *
 * TODO dispatch based on sequence type, i.e., return associative source sequences
 *      as is since they cannot have duplicates
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Sequence
 *     the sequence whose duplicates shall be removed
 */
template<
	typename Sequence
>
struct RemoveDuplicates {
	BOOST_MPL_ASSERT(( boost::mpl::is_sequence< Sequence > ));

	typedef typename boost::mpl::copy<
		Sequence,
		boost::mpl::inserter<    // inserter for sets
			boost::mpl::set<>,   // starting with an empty set
			boost::mpl::insert< boost::mpl::_1, boost::mpl::_2 >
		>
	>::type type;
};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_REMOVEDUPLICATES_HPP_ */
