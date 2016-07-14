/*
 * Function.hpp
 *
 *  Created on: Jan 27, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_FUNCTION_HPP_
#define LIBCPP_TYPES_FUNCTION_HPP_


#ifdef USE_BOOST_FUNCTION
// use boost function

#include <boost/function.hpp>

namespace ns_types {


template<
	typename ReturnType,
	typename... Arguments
>
using Function = boost::function< ReturnType( Arguments... ) >;

} /* end namespace ns_types */

#else
// use standard function

#include <functional>

namespace ns_types {


template<
	typename ReturnType,
	typename... Arguments
>
using Function = std::function< ReturnType( Arguments... ) >;


} /* end namespace ns_types */

#endif


#endif /* LIBCPP_TYPES_FUNCTION_HPP_ */
