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
