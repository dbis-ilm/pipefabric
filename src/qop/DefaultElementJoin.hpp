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
