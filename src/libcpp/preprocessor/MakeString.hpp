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
