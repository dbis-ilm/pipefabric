/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef ElementJoinTraits_hpp_
#define ElementJoinTraits_hpp_

#include <type_traits>

namespace pfabric {

/**
 * @brief A common interface for algorithms that join two stream elements.
 *
 * @tparam ElementJoinImpl
 *    the ElementJoin implementation for which the traits are defined
 *
 * Specialize the class for all ElementJoin implementations that do not implement the
 * default interface.
 */
template<
	typename ElementJoinImpl
>
class ElementJoinTraits {
private:

	/// the ElementJoin component whose traits are defined here
	typedef ElementJoinImpl ElementJoin;

public:

	//////   public types   //////
	typedef typename ElementJoin::ResultElement ResultElement;

	//////   public constants   //////


	//////   public interface   //////

	/**
	 * @brief Join two stream elements.
	 *
	 * @tparam LeftStreamElement
	 *    the type of the left stream element having the first attributes
	 * @tparam RightStreamElement
	 *    the type of the right stream element having the last attributes
	 * @param[in] leftElement
	 *    the left element of the join having the first attributes
	 * @param[in] rightElement
	 *    the right element of the join having the last attributes
	 * @return a new joint view on both element instances comprising all attributes
	 */
	template<
		typename LeftStreamElement,
		typename RightStreamElement
	>
	static ResultElement joinElements( LeftStreamElement&& leftElement, RightStreamElement&& rightElement ) {
		return ElementJoin::joinElements(
			std::forward< LeftStreamElement >( leftElement ),
			std::forward< RightStreamElement >( rightElement )
		);
	}
};

} /* end namespace pfabric */


#endif /* ElementJoinTraits_hpp_ */
