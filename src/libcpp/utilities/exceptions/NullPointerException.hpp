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
