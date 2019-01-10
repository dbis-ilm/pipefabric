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

/*
 * SharedPtr.hpp
 *
 *  Created on: Jun 5, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_SHAREDPTR_HPP_
#define LIBCPP_TYPES_SHAREDPTR_HPP_

#include <type_traits>


// determine the shared pointer type
#ifdef USE_BOOST_SMART_PTR

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>


namespace ns_types {

/**
 * @brief A weak pointer type.
 *
 * This type maps to @c boost::weak_ptr.
 * @tparam T the element type that is wrapped in a weak pointer
 */
template< typename T >
using WeakPtr = boost::weak_ptr< T >;

/**
 * @brief A shared pointer type.
 *
 * This type maps to @c boost::shared_ptr.
 * @tparam T the element type that is wrapped in a shared pointer
 */
template< typename T >
using SharedPtr = boost::shared_ptr< T >;

/**
 * @brief A type for allowing the access of the @c this pointer of an object with
 *        shared ownership semantics through a @c SharedPointer.
 *
 * This type maps to @c boost::enable_shared_from_this.
 * @tparam T the element type that shall be accessible via a shared @c this pointer
 */
template< typename T >
using EnableSharedFromThis = boost::enable_shared_from_this< T >;


/**
 * @brief Allocate a new object with shared reference semantics.
 *
 * This method allocates the memory for the requested object and its reference
 * counter with one call in order to minimize the allocation overhead.
 *
 * @see std::make_shared()
 */
template< class T, class... Args >
inline SharedPtr<T> make_shared( Args&&... args ) {
	return boost::make_shared< T >( std::forward< Args >( args )... );
}

} /* end namespace ns_types */

#else // endif USE_BOOST_SMART_PTR

#include <memory>

namespace ns_types {

/**
 * @brief A shared pointer type.
 *
 * This type maps to @c std::shared_ptr.
 * @tparam T the element type that is wrapped in a shared pointer
 */
template< typename T >
using SharedPtr = std::shared_ptr< T >;

/**
 * @brief A weak pointer type.
 *
 * This type maps to @c std::weak_ptr.
 * @tparam T the element type that is wrapped in a weak pointer
 */
template< typename T >
using WeakPtr = std::weak_ptr< T >;

/**
 * @brief A type for allowing the access of the @c this pointer of an object with
 *        shared ownership semantics through a @c SharedPointer.
 *
 * This type maps to @c std::enable_shared_from_this.
 * @tparam T the element type that shall be accessible via a shared @c this pointer
 */
template< typename T >
using EnableSharedFromThis = std::enable_shared_from_this< T >;


/**
 * @brief Allocate a new object with shared reference semantics.
 *
 * This method allocates the memory for the requested object and its reference
 * counter with one call in order to minimize the allocation overhead.
 *
 * @see std::make_shared()
 */
template< class T, class... Args >
inline SharedPtr<T> make_shared( Args&&... args ) {
	return std::make_shared< T >( std::forward< Args >( args )... );
}

} /* end namespace ns_types */

#endif // endif !USE_BOOST_SMART_PTR


#endif /* LIBCPP_TYPES_SHAREDPTR_HPP_ */
