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
 * GetTypeName.hpp
 *
 *  Created on: Jan 7, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_GETTYPENAME_HPP_
#define LIBCPP_UTILITIES_GETTYPENAME_HPP_

#include <boost/core/typeinfo.hpp>
#include <boost/typeof/typeof.hpp>


namespace ns_utilities {

namespace result_of {

/**
 * @brief Helper meta function returning the type of the getTypeName function.
 *
 * This meta function is required since GCC doesn't support typeid expressions
 * that are required for determining the result type of this function via decltype.
 */
template<
	typename Type
>
struct getTypeName {
	typedef BOOST_TYPEOF( boost::core::demangled_name( BOOST_CORE_TYPEID( Type ) ) ) type;
};

} /* end namespace result_of */


/**
 * @brief Function that gets the name of a type.
 *
 * @see @c boost::core::demangled_name
 * @tparam Type the Type whose name shall be extracted
 * @return the demangled name of the type passed as template argument
 */
template<
	typename Type
>
constexpr typename result_of::getTypeName< Type >::type getTypeName() {
	return boost::core::demangled_name( BOOST_CORE_TYPEID( Type ) );
}

} /* end namespace ns_utilities */


#endif /* LIBCPP_UTILITIES_GETTYPENAME_HPP_ */
