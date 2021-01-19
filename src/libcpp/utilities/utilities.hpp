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
 * utilities.hpp
 *
 *  Created on: Jun 16, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_SOURCES_INCLUDE_LIBCPP_UTILITIES_UTILITIES_HPP_
#define LIBCPP_SOURCES_INCLUDE_LIBCPP_UTILITIES_UTILITIES_HPP_

/// include all utilities

/// time-related stuff
#include "libcpp/utilities/ConvertTimeDurationToDouble.hpp"
#include "libcpp/utilities/Timer.hpp"

/// compile-time stuff
#include "libcpp/utilities/GetTypeName.hpp"
#include "libcpp/utilities/TypePrinter.hpp"

/// miscellaneous stuff
#include "libcpp/utilities/BindVariadic.hpp"
#include "libcpp/utilities/DynamicPointerMove.hpp"
#include "libcpp/utilities/PrintCSV.hpp"
#include "libcpp/utilities/exceptions.hpp"
#include "libcpp/utilities/EnvironmentVariable.hpp"

#endif /* LIBCPP_SOURCES_INCLUDE_LIBCPP_UTILITIES_UTILITIES_HPP_ */
