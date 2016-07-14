/*
 * DefaultElementJoin.hpp
 *
 *  Created on: Mar 15, 2015
 *      Author: fbeier
 */

#ifndef DEFAULTELEMENTJOIN_HPP_
#define DEFAULTELEMENTJOIN_HPP_


#ifdef USE_LAZY_ELEMENT_JOIN

#include "qop/LazyElementJoin.hpp"

namespace pfabric {

/**
 * @brief Alias for using a lazy element join algorithm by default.
 *
 * @tparam LeftStreamElement
 *    the type of the left stream element having the first attributes
 * @tparam RightStreamElement
 *    the type of the right stream element having the last attributes
 */
template<
	typename LeftStreamElement,
	typename RightStreamElement
>
using DefaultElementJoin = LazyElementJoin< LeftStreamElement, RightStreamElement >;

} /* end namespace pfabric */

#else

#include "qop/EagerElementJoin.hpp"
#include "core/TuplePtrFactory.hpp"

namespace pfabric {

/**
 * @brief Alias for using a lazy element join algorithm by default.
 *
 * @tparam LeftStreamElement
 *    the type of the left stream element having the first attributes
 * @tparam RightStreamElement
 *    the type of the right stream element having the last attributes
 * @tparam StreamElementFactory
 *    a factory class for creating result element instances from attributes,
 *    default: @c TuplePtrFactory for tuple creation on the heap via intrusive pointers
 */
template<
	typename LeftStreamElement,
	typename RightStreamElement,
	typename StreamElementFactory = TuplePtrFactory
>
using DefaultElementJoin = EagerElementJoin< LeftStreamElement, RightStreamElement, StreamElementFactory >;

} /* end namespace pquery */

#endif

#endif /* DEFAULTELEMENTJOIN_HPP_ */
