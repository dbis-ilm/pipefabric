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
 * DynamicPointerMove.hpp
 *
 *  Created on: Mar 12, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_DYNAMICPOINTERMOVE_HPP_
#define LIBCPP_UTILITIES_DYNAMICPOINTERMOVE_HPP_

#include "libcpp/types/detail/UniquePtr.hpp"

#include <utility>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/assert.hpp>

namespace ns_utilities {

/**
 * @brief Helper function that allows converting a @c ns_types::UniquePtr from one type to another
 *        in a class hierarchy.
 *
 * The @c SourceType and @c TargetType must be in the same type hierarchy. The function allows
 * hierarchy upcasts as well as downcasts, while the latter might fail with a @c std::bad_cast
 * exception if the @c dynamic_cast() operation fails.
 *
 * @tparam TargetType
 *           the requested target type of the pointer
 * @tparam SourceType
 *           the current type of the pointer
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename TargetType,
	typename SourceType
>
ns_types::UniquePtr< TargetType > dynamicPointerMove( ns_types::UniquePtr< SourceType >& src ) {
	BOOST_MPL_ASSERT(( boost::mpl::or_<
			// allow hierarchy upcast (always safe)
			boost::is_base_of< SourceType, TargetType >,
			// allow hierarchy downcast (might fail)
			boost::is_base_of< TargetType, SourceType >
		>
	));
	ns_types::UniquePtr< TargetType > target;

	SourceType* srcPtr = src.get();
	if( srcPtr != NULL ) {
		// throws std::bad_cast if illegal conversion is tried here
		// (possible when downcasting from base reference to class sibling)
		target.reset( &dynamic_cast<TargetType&>( *srcPtr ) );
		src.release();
	}

	return std::move(target);
}

} /* end namespace ns_utilities */


#endif /* LIBCPP_UTILITIES_DYNAMICPOINTERMOVE_HPP_ */
