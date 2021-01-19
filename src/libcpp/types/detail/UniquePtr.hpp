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
 * UniquePtr.hpp
 *
 *  Created on: Jun 5, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_UNIQUEPTR_HPP_
#define LIBCPP_TYPES_UNIQUEPTR_HPP_


#include <memory>
#include <utility>


namespace ns_types {

/**
 * @brief A shared pointer type.
 *
 * This type maps to @c boost::shared_ptr.
 * @tparam T the element type that is wrapped in a shared pointer
 */
template< typename T >
using UniquePtr = std::unique_ptr< T >;


/**
 * @brief Create a unique pointer to a new instance of type @c T.
 *
 * This method exists until make_unique is introduced in C++14.
 * It requires that @c T's has a constructor accepting @c Args as argument
 * exists and is publicly accessible.
 *
 * @tparam T the type of the requested instance
 * @tparam Args parameter types which are forwarded to T's constructor
 * @param[in] args parameter list which are forwarded to T's constructor
 */
template< class T, typename... Args >
std::unique_ptr< T > make_unique( Args&&... args ) {
	return std::move( std::unique_ptr< T >( new T( std::forward<Args>(args)... ) ) );
}

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_UNIQUEPTR_HPP_ */
