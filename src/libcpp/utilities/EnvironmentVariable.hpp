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
 * EnvironmentVariable.hpp
 *
 *  Created on: Jan 8, 2015
 *      Author: felix
 */

#ifndef LIBCPP_UTILITIES_ENVIRONMENTVARIABLE_HPP_
#define LIBCPP_UTILITIES_ENVIRONMENTVARIABLE_HPP_

#include <cstdlib>
#include <string>
#include <utility>
#include <boost/optional/optional.hpp>
#include <boost/none_t.hpp>


namespace ns_utilities {

/**
 * An operating system environment variable which may or may not be set.
 */
class EnvironmentVariable
	: public boost::optional< std::string > {
private:

	/// the optional type used to hold the environment variable value
	typedef boost::optional< std::string > ValueImpl;

public:

	/**
	 * @brief Get an environment variable from the system.
	 *
	 * @param[in] name
	 *     the name of the environment variable
	 * @return the environment variable with the requested name if set,
	 *         an unset environment variable otherwise
	 */
	static EnvironmentVariable getEnvironmentVariable( const std::string& name ) {
		return getEnvironmentVariable( name.c_str() );
	}

	/**
	 * @brief Get an environment variable from the system.
	 *
	 * @param[in] name
	 *     the name of the environment variable
	 * @return the environment variable with the requested name if set,
	 *         an unset environment variable otherwise
	 */
	static EnvironmentVariable getEnvironmentVariable( const char* name ) {
		ValueImpl value = boost::none;
		const std::string nameStr = name != nullptr ? name : "";

		if( nameStr != "" ) {
			char* env = getenv( name );

			if( env != nullptr ) {
				value = std::string( env );
			}
		}

		return EnvironmentVariable( name, value );
	}


	/**
	 * @brief Get the name of the environment variable.
	 *
	 * @return the name of the environment variable
	 */
	const std::string& getName() const {
		return mName;
	}


private:

	/**
	 * @brief construct an environment variable without a value set.
	 *
	 * @param[in] name
	 *     the name of the environment variable
	 */
	EnvironmentVariable( const char* name, ValueImpl value = boost::none_t() ) :
		ValueImpl( value ), mName( name != nullptr ? name : "" ) {

	}

	std::string mName; /**< the name of the environment variable */
};


} /* end namespace ns_utilities */


/**
 * @brief Get an environment variable from the system.
 *
 * @tparam NameType
 *     the type of the environment variable name
 * @param[in] name
 *     the name of the environment variable
 * @return the environment variable with the requested name if set,
 *         an unset environment variable otherwise
 */
template< typename NameType >
ns_utilities::EnvironmentVariable getEnvironmentVariable( NameType&& name ) {
	return ns_utilities::EnvironmentVariable::getEnvironmentVariable( std::forward< NameType >( name ) );
}


#endif /* LIBCPP_UTILITIES_ENVIRONMENTVARIABLE_HPP_ */
