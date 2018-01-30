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
 * InsertAssertUnique.hpp
 *
 *  Created on: Jul 8, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_INSERTASSERTUNIQUE_HPP_
#define LIBCPP_MPL_SEQUENCES_INSERTASSERTUNIQUE_HPP_

#include <boost/mpl/key_type.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/assert.hpp>


namespace ns_mpl {

/**
 * @brief Insertion algorithm for associative sequences throwing a compile-time
 *        assertion when duplicates are tried to be inserted.
 *
 * This insertion meta function tries to extend an associative @c Sequence with
 * a new @c Entry type. The @c Entry is only inserted in case it is unique
 * within the @c Sequence. Otherwise, the compilation will fail with a
 * compile-time assertion. This is useful to make sure that compile-time type
 * entries do not get overwritten silently when this is considered a bug.
 *
 * TODO Generalize this for all sequences, e.g., vectors, and not only associative ones.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Sequence
 *     an associative sequence that shall be extended
 * @tparam Entry
 *     the entry to be inserted in the sequence
 */
template<
	typename Sequence,
	typename Entry
>
struct InsertAssertUnique {
	// determine the key the entry would have in the associative sequence
	// this can differ, e.g., between sets and maps
	typedef typename boost::mpl::key_type< Sequence, Entry >::type Key;

	// make sure that this key does not occur in the sequence yet
	BOOST_MPL_ASSERT_MSG(
		(boost::mpl::not_< boost::mpl::has_key< Sequence, Key > >::value),
		NO_DUPLICATE_ENTRIES_ALLOWED,
		(Key, Sequence)
	);

	// finally, insert the entry
	typedef typename boost::mpl::insert< Sequence, Entry >::type type;
};

} /* end namespace ns_mpl */

#endif /* LIBCPP_MPL_SEQUENCES_INSERTASSERTUNIQUE_HPP_ */
