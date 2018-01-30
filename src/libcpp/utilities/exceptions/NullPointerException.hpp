/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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
 * NullPointerException.hpp
 *
 *  Created on: Apr 29, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_NULLPOINTEREXCEPTION_HPP_
#define LIBCPP_UTILITIES_NULLPOINTEREXCEPTION_HPP_

namespace ns_utilities {
namespace exceptions {

/**
 * @brief An Exception that indicates that an unexpected null pointer was encountered.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
struct NullPointerException :
	public ExceptionBase
{};

} /* end namespace exceptions */
} /* end namespace ns_utilities */


#endif /* LIBCPP_UTILITIES_NULLPOINTEREXCEPTION_HPP_ */
