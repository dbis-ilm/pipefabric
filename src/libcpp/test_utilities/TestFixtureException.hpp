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