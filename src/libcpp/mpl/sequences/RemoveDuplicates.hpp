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
