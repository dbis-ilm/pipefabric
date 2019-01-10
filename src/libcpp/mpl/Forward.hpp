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
