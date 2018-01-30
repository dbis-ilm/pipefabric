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
 * GetDuplicates.hpp
 *
 *  Created on: Jul 8, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_GETDUPLICATES_HPP_
#define LIBCPP_MPL_SEQUENCES_GETDUPLICATES_HPP_

#include "RemoveDuplicates.hpp"

#include <boost/mpl/count.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/int.hpp>


namespace ns_mpl {

/**
 * @brief Meta function which gets all duplicate types in a sequence.
 *
 * This meta function scans the @c Sequence and returns a new sequence comprising
 * all types that occurred at least twice in the source @c Sequence.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Sequence
 *     the sequence whose duplicates shall be returned
 */
template<
	typename Sequence
>
struct GetDuplicates {
	// filter all duplicate types (they will occur as many times in the result
	// vector as they occur in the sequence)
	typedef typename boost::mpl::filter_view< // select all types in
		Sequence,                // the source sequence
		boost::mpl::greater<     // which occur more than once in the source sequence
			boost::mpl::count< Sequence, boost::mpl::_1 >,
			boost::mpl::int_<1>
		>
	>::type DuplicatesWithRepetitions;

	// drop all duplicates in the resulting duplicate type vector to have
	// each duplicate type exactly once
	typedef typename RemoveDuplicates< DuplicatesWithRepetitions >::type type;
};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_GETDUPLICATES_HPP_ */
