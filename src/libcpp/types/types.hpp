/*
 * types.hpp
 *
 *  Created on: Jun 5, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TYPES_HPP_
#define LIBCPP_TYPES_TYPES_HPP_


/// include all types

////// smart pointers
#include "libcpp/types/detail/Function.hpp"
#include "libcpp/types/detail/SharedPtr.hpp"
#include "libcpp/types/detail/SharedInstance.hpp"
#include "libcpp/types/detail/IntrusivePtr.hpp"
#include "libcpp/types/detail/UniquePtr.hpp"
#include "libcpp/types/detail/UniqueInstance.hpp"

////// misc
#include "libcpp/types/detail/TupleType.hpp"
#include "libcpp/types/detail/SubstringRef.hpp"


/// include all type traits

#include "libcpp/types/traits/PointerTraits.hpp"
#include "libcpp/types/traits/GetPointedElementType.hpp"
#include "libcpp/types/traits/FunctionTraits.hpp"

#endif /* LIBCPP_TYPES_TYPES_HPP_ */
