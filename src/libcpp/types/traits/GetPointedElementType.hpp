/*
 * GetPointedElementType.hpp
 *
 *  Created on: Feb 16, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TRAITS_GETPOINTEDELEMENTTYPE_HPP_
#define LIBCPP_TYPES_TRAITS_GETPOINTEDELEMENTTYPE_HPP_


namespace ns_types {

/**
 * @brief Meta function to extract the pointed-to element type of a complex pointer class.
 *
 * @tparam Pointer
 *    the pointer class (must provide a dereferencing operator)
 */
template< typename Pointer >
struct getPointedElementType
{
	typedef typename std::remove_cv<
		typename std::remove_reference< Pointer >::type
	>::type _P;

	typedef typename std::remove_reference<
		typename std::result_of<
			decltype( &_P::operator* ) ( _P* )
		>::type
	>::type type;
};

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_TRAITS_GETPOINTEDELEMENTTYPE_HPP_ */
