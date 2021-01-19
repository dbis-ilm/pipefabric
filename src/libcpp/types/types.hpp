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
