/*
 * MakeStrings.hpp
 *
 *  Created on: Jun 2, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_PREPROCESSOR_MAKESTRINGS_HPP_
#define LIBCPP_PREPROCESSOR_MAKESTRINGS_HPP_


#include "MakeString.hpp"

#include <iterator>
#include <type_traits>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/transformed.hpp>


/**
 * @brief A macro for converting a container a @c std::string.
 *
 * This macro converts @c container into a range of @c std::strings.
 * Therefore, it converts each element into a string using the @c MAKE_STRING macro
 * and offers a transformed view onto the original container.
 */
#define MAKE_STRINGS( container ) \
	boost::make_iterator_range( std::begin( container ), std::end( container ) ) \
		| boost::adaptors::transformed( [](	const \
			typename std::remove_cv< \
				typename std::remove_reference< decltype( container ) >::type \
			>::type::value_type& path ) { \
				return MAKE_STRING( path ); \
			} \
		)


#endif /* SUBMODULES_LIBCPP_PREPROCESSOR_MAKESTRINGS_HPP_ */
