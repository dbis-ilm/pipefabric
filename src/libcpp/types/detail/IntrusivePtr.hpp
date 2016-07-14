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
