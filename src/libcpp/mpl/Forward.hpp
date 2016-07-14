/*
 * Forward.hpp
 *
 *  Created on: Jul 10, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_FORWARD_HPP_
#define LIBCPP_MPL_FORWARD_HPP_

#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/static_assert.hpp>

namespace ns_mpl {

/**
 * @brief Constexpr fix for the std::forward function.
 *
 * Specialization for lvalue reference parameters.
 *
 * @param[in] parm
 *     the parameter to be forwarded
 * @return
 *     an rvalue reference to the parameter
 */
template<
	typename Parameter
>
inline constexpr Parameter&&
forward( typename std::remove_reference< Parameter >::type& parm ) {
	return static_cast< Parameter&& >( parm );
}

/**
 * @brief Constexpr fix for the std::forward function.
 *
 * Specialization for rvalue reference parameters.
 *
 * @param[in] parm
 *     the parameter to be forwarded
 * @return
 *     an rvalue reference to the parameter
 */
template<
	typename Parameter
>
inline constexpr Parameter&&
forward( typename std::remove_reference< Parameter >::type&& parm ) {
	BOOST_STATIC_ASSERT_MSG( boost::is_lvalue_reference< Parameter >::value,
		"cannot forward an lvalue reference as rvalue reference"
	);
	return static_cast< Parameter&& >( parm );
}

} /* end namespace ns_mpl */


#endif /* LIBCPP_FORWARD_HPP_ */
