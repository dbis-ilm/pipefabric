/*
 * SequenceJoiner.hpp
 *
 *  Created on: Dec 31, 2014
 *      Author: felix
 */

#ifndef LIBCPP_MPL_SEQUENCES_SEQUENCEJOINER_HPP_
#define LIBCPP_MPL_SEQUENCES_SEQUENCEJOINER_HPP_

#include <boost/mpl/inserter.hpp>
#include <boost/mpl/empty_sequence.hpp>
#include <boost/mpl/joint_view.hpp>


namespace ns_mpl {

/**
 * @brief An inserter for joining sequences together.
 *
 * This inserter class can be used for joining multiple mpl sequences together. It basically
 * creates joint views over all of them.
 *
 * @tparam Sequence
 *     the initial sequence for starting the joins, default empty
 */
template<
	typename Sequence = boost::mpl::empty_sequence
>
struct SequenceJoiner :
	public boost::mpl::inserter<
		Sequence,
		boost::mpl::joint_view< boost::mpl::_1, boost::mpl::_2 >
	>
{};

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_SEQUENCES_SEQUENCEJOINER_HPP_ */
