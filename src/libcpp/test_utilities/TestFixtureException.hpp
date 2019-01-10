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
 * TestFixtureException.hpp
 *
 *  Created on: May 27, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TEST_UTILITIES_TESTFIXTUREEXCEPTION_HPP_
#define LIBCPP_TEST_UTILITIES_TESTFIXTUREEXCEPTION_HPP_

#include "libcpp/utilities/exceptions.hpp"


namespace ns_test_utilities {
namespace exceptions {

/**
 * @brief Base class for all Exceptions that might be thrown by the test fixture module.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
struct TestFixtureException :
	public ns_utilities::exceptions::ExceptionBase
{};

} /* end namespace exceptions */
} /* end namespace ns_test_utilities */



#endif /* LIBCPP_TEST_UTILITIES_TESTFIXTUREEXCEPTION_HPP_ */
