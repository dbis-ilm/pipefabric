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
