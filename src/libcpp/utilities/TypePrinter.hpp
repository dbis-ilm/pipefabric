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
 * TypePrinter.hpp
 *
 *  Created on: Jul 7, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_UTILITIES_TYPEPRINTER_HPP_
#define LIBCPP_UTILITIES_TYPEPRINTER_HPP_

#include "GetTypeName.hpp"
#include "libcpp/mpl/algorithms/StaticForEach.hpp"

#include <iostream>
#include <typeinfo>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/not.hpp>


namespace ns_utilities {

struct TypePrinter;

namespace impl {

/**
 * @brief A type printer implementation type.
 *
 * Forward declaration of a meta function class that can print type information
 * in a human-readable way. This is required since partial template method
 * specializations are not allowed for specific types to be printed which would
 * be required for @c TypePrinter::apply().
 *
 * @tparam Type
 *     the type to be printed
 * @tparam Enable
 *     parameter for selecting the matching template specialization
 */
template< typename Type, class Enable = void >
struct TypePrinterImpl;


/**
 * @brief Generic base case specialization for simple types.
 *
 * This specialization is instantiated when a simple type is passed as template argument.
 * It prints out the type name that can be obtained via RTTI.
 *
 * @tparam Type
 *     the type to be printed
 */
template<
	typename Type
>
struct TypePrinterImpl<
	Type,
	typename boost::enable_if<
		boost::mpl::not_< boost::mpl::is_sequence< Type > >
	>::type
> {
	static void apply() {
		std::cout << getTypeName<Type>() << ", ";
	}
};

#if 0
/**
 * @brief Specialization for integral compile-time constant types.
 *
 * This specialization is instantiated for compile-time integral constants.
 * It prints out the actual value of the type, not its name.
 *
 * @tparam IntegralType
 *     the integral base type
 * @tparam Value
 *     the constant value of the type
 */
template<
	typename IntegralType,
	IntegralType Value
>
struct TypePrinterImpl<
		boost::mpl::integral_c< IntegralType, Value >
	>
{
	static void apply() {
		std::cout << "const " << boost::mpl::integral_c< IntegralType, Value >::value << ", ";
	}
};
#endif


/**
 * @brief Generic base case specialization for complex mpl types.
 *
 * This specialization is instantiated when a complex type (an mpl::sequence) is passed as argument.
 * It recursively prints out all types contained in the complex type.
 *
 * @tparam Type
 *     the type to be printed
 */
template<
	typename Type
>
struct TypePrinterImpl<
		Type,
		typename boost::enable_if<
			boost::mpl::is_sequence< Type >
		>::type
	>
{
	static void apply() {
		std::cout << "mpl::seq< ";
		ns_mpl::staticForEach< Type, TypePrinter >();
		std::cout << " >, ";
	}
};

} /* end namespace impl */


/**
 * @brief Meta function class which prints the type name of its argument.
 *
 * This meta function class prints the name of the type passed as argument to
 * its nested @c apply meta function to the standard output stream.
 * It can be invoked without an actual instance and can therefore be used
 * in @c staticForEach.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
struct TypePrinter {
	// function for static_for_each (without an instance)
	template< typename Type >
	static void apply() {
		// forward the call to the actual type printer implementation
		impl::TypePrinterImpl< Type >::apply();
	}
};

} /* end namespace ns_utilities */


#endif /* LIBCPP_UTILITIES_TYPEPRINTER_HPP_ */
