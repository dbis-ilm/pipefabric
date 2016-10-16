/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef DefaultElementJoin_hpp_
#define DefaultElementJoin_hpp_


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

}

#endif

#endif /* DefaultElementJoin_hpp_ */
