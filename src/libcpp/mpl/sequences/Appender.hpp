/*
 * Appender.hpp
 *
 *  Created on: Jul 8, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_SEQUENCES_APPENDER_HPP_
#define LIBCPP_MPL_SEQUENCES_APPENDER_HPP_

#include "Append.hpp"

#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/push_back.hpp>

namespace ns_mpl {

/**
 * @brief An inserter for appending types/sequences to another one.
 *
 * This type implements an inserter which can e.g., used in mpl algorithms to
 * extend sequences with other sequence/non-sequence types.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam Element
 *     the current element to be inserted
 * @tparam State
 *     the current state of the inserter with the elements collected so far
 * @tparam InitialState
 *     the initial state when the inserter is instantiated
 */
template<
	typename Element,
	typename State,
	typename InitialState
>
struct Appender :
		boost::mpl::inserter<  // is an inserter
			InitialState,      // which extends the initial sequence
			Append<            // with appending
				Element,       // the current element
				State          // to the state processed so far
			>
		>
{};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_APPENDER_HPP_ */
