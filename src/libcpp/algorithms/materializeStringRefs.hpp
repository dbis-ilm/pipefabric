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
 * materializeStringRefs.hpp
 *
 *  Created on: Apr 29, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_ALGORITHMS_MATERIALIZESTRINGREFS_HPP_
#define LIBCPP_ALGORITHMS_MATERIALIZESTRINGREFS_HPP_

#include "libcpp/types/detail/SubstringRef.hpp"

#include <type_traits>
#include <vector>
#include <algorithm>
#include <functional>


namespace ns_algorithms {

/**
 * @brief Function converting a collection of substring references into a collection of strings.
 *
 * This function will convert all substring references passed via the @c stringRefs
 * parameter into their corresponding strings and appends them to the @c results container.
 *
 * @param[in] stringRefs
 *    the vector of substring references
 * @return a vector of transformed strings corresponding to the substring references
 */
template<
	typename StringRefs,
	typename Container
>
void materializeStringRefs( StringRefs&& stringRefs, Container& results ) {
	typedef typename std::remove_cv<
		typename std::remove_reference<
			StringRefs
		>::type
	>::type::value_type StringRef;

	std::transform( stringRefs.begin(), stringRefs.end(), std::back_inserter( results ),
		std::bind( &StringRef::operator*, std::placeholders::_1 )
	);
}


/**
 * @brief Function converting a collection of substring references into a vector of strings.
 *
 * @param[in] stringRefs
 *    the vector of substring references
 * @return a vector of transformed strings corresponding to the substring references
 */
template<
	typename StringRefs
>
std::vector< std::string > materializeStringRefs( StringRefs&& stringRefs ) {
	std::vector< std::string > strings;

	materializeStringRefs( std::forward< StringRefs >( stringRefs ), strings );
	return strings;
}

} /* end namespace ns_algorithms */


#endif /* LIBCPP_ALGORITHMS_MATERIALIZESTRINGREFS_HPP_ */
