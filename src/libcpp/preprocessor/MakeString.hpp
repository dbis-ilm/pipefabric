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
 * MakeString.hpp
 *
 *  Created on: Aug 15, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_PREPROCESSOR_MAKESTRING_HPP_
#define LIBCPP_PREPROCESSOR_MAKESTRING_HPP_

#include <sstream>

/**
 * @brief A macro for converting an expression into a @c std::string.
 *
 * This macro converts the argument @a x into a @c std::string. A temporary @c std::stringstream
 * instance is created and the @a x content is streamed into it before it is finally returned.
 */
#define MAKE_STRING( x ) \
  (( dynamic_cast< std::ostringstream & >( \
         std::ostringstream().seekp( 0, std::ios_base::cur ) << x ) \
    ).str() )

#endif /* LIBCPP_PREPROCESSOR_MAKESTRING_HPP_ */
