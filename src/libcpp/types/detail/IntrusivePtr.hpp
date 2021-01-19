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
 * IntrusivePtr.hpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_INTRUSIVEPTR_HPP_
#define LIBCPP_TYPES_INTRUSIVEPTR_HPP_

#include <boost/intrusive_ptr.hpp>

namespace ns_types {

/**
 * @brief An intrusive pointer type.
 *
 * This type represents a pointer to an object having an embedded reference count.
 * It forwards to the boost implementation.
 *
 * @tparam T
 *    the type the pointer should point to
 */
template< typename T >
using IntrusivePtr = boost::intrusive_ptr< T >;

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_INTRUSIVEPTR_HPP_ */
