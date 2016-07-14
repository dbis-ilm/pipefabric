/*
 * ElementJoinTraits.hpp
 *
 *  Created on: Mar 16, 2015
 *      Author: fbeier
 */

#ifndef ELEMENTJOINTRAITS_HPP_
#define ELEMENTJOINTRAITS_HPP_

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
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
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

} /* end namespace pquery */


#endif /* ELEMENTJOINTRAITS_HPP_ */
