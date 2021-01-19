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
 * BindVariadic.hpp
 *
 *  Created on: Jan 15, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_BINDVARIADIC_HPP_
#define LIBCPP_UTILITIES_BINDVARIADIC_HPP_

#include "libcpp/mpl/sequences/GenerateIndexes.hpp"

#include <boost/bind/bind.hpp>

namespace ns_utilities {

namespace impl {

/**
 * @brief Helper function that correctly binds the required number of arguments
 *        to the function object from a variable argument list.
 *
 * The challenge here is to bind the variadic argument list correctly. If the
 * number of arguments is known, placeholders as _1, _2, etc. can be used for
 * the function arguments. Therefore, meta-programming is used to determine
 * the number of arguments during compile time and generate a placeholder for
 * each of them. Since the placeholder _1 is just an alias for boost::arg< i >,
 * it is sufficient to to instantiate it as sequence with all required argument
 * indices @i.
 *
 * @tparam ReturnType
 *     the return type of the function to be bound
 * @tparam ClassType
 *     the type of the class containing the member function
 * @tparam Args
 *     the variadic type pack with types of the function arguments
 * @tparam Indexes
 *     a consecutive integer sequence for each argument number in @c Args
 *
 * @param[in] memberFn
 *     function pointer with the address of the member function in the class definition to be bound
 * @param[in] instance
 *     the instance of the class whose member function should be bound into the function object
 * @param[in] indexes
 *     a tuple with the same number of @c int template arguments as there are types in
 *     the variadic @c Args type pack
 * @return a function object for the bound function
 */
template<
  typename ReturnType,
  class ClassType,
  class... Args,
  int... Indexes
>
auto bindVariadicImpl( ReturnType (ClassType::* memberFn) (Args...), ClassType* instance,
    ns_mpl::IndexTuple< Indexes... > indexes )
  -> decltype(
    boost::bind( memberFn, instance, boost::arg< Indexes >()... )
  )
{
    return boost::bind( memberFn, instance,
        // generate a placeholder for each index in the Indexes list
        // using parameter pack expansion
    boost::arg< Indexes >()...
  );
}

/**
 * @brief bindVariadicImpl overload for const member functions.
 *
 * @tparam ReturnType
 *     the return type of the function to be bound
 * @tparam ClassType
 *     the type of the class containing the member function
 * @tparam Args
 *     the variadic type pack with types of the function arguments
 * @tparam Indexes
 *     a consecutive integer sequence for each argument number in @c Args
 *
 * @param[in] memberFn
 *     function pointer with the address of the member function in the class definition to be bound
 * @param[in] instance
 *     the instance of the class whose member function should be bound into the function object
 * @param[in] indexes
 *     a tuple with the same number of @c int template arguments as there are types in
 *     the variadic @c Args type pack
 * @return a function object for the bound function
 */
template<
  typename ReturnType,
  class ClassType,
  class... Args,
  int... Indexes
>
auto bindVariadicImpl( ReturnType (ClassType::* memberFn) (Args...) const, const ClassType* instance,
    ns_mpl::IndexTuple< Indexes... > indexes )
  -> decltype(
    boost::bind( memberFn, instance, boost::arg< Indexes >()... )
  )
{
    return boost::bind( memberFn, instance,
    // generate a placeholder for each index in the Indexes list
    // using parameter pack expansion
    boost::arg< Indexes >()...
  );
}

} /* end namespace impl */


/**
 * @brief Bind a member function with variable argument list.
 *
 * This method binds a class' member function having a variable argument list
 * to an instance of that respective class and returns a function object for it.
 *
 * Idea adapted from: https://groups.google.com/forum/#!topic/boost-list/J9hoc81Rx-E
 *                    http://www.crest.iu.edu/~dgregor/cpp/variadic-templates.pdf
 *
 * @tparam ReturnType
 *     the return type of the function to be bound
 * @tparam ClassType
 *     the type of the class containing the member function
 * @tparam Args
 *     the variadic type pack with types of the function arguments
 *
 * @param[in] memberFn
 *     function pointer with the address of the member function in the class definition to be bound
 * @param[in] instance
 *     the instance of the class whose member function should be bound into the function object
 * @return a function object for the bound function
 */
template<
  typename ReturnType,
  class ClassType,
  typename ... Args
>
auto bindVariadic( ReturnType (ClassType::* memberFn) (Args...), ClassType* instance )
  -> decltype(
    impl::bindVariadicImpl< ReturnType >(
      memberFn, instance, typename ns_mpl::generateIndexes< sizeof...(Args), 1 >::type()
    )
  )
{
    return impl::bindVariadicImpl< ReturnType >( memberFn, instance,
    // determine the number of provided arguments during compile time
    // with generating an appropriate integer tuple with generateIndexes
    // and provide a default constructed instance of that type to
    // instantiate the correct implementation of the actual bind helper
    // function that binds all arguments correctly
    typename ns_mpl::generateIndexes<
      sizeof...(Args), // for each argument in the list
      1                // starting with placeholder index 1
    >::type()
    );
}

/**
 * @brief bindVariadic overload for const member functions.
 *
 * @tparam ReturnType
 *     the return type of the function to be bound
 * @tparam ClassType
 *     the type of the class containing the member function
 * @tparam Args
 *     the variadic type pack with types of the function arguments
 *
 * @param[in] memberFn
 *     function pointer with the address of the member function in the class definition to be bound
 * @param[in] instance
 *     the instance of the class whose member function should be bound into the function object
 * @return a function object for the bound function
 */
template<
  typename ReturnType,
  class ClassType,
  typename ... Args
>
auto bindVariadic( ReturnType (ClassType::* memberFn) (Args...) const, const ClassType* instance )
  -> decltype(
    impl::bindVariadicImpl< ReturnType >(
      memberFn, instance, typename ns_mpl::generateIndexes< sizeof...(Args), 1 >::type()
    )
  )
{
    return impl::bindVariadicImpl< ReturnType >( memberFn, instance,
    // determine the number of provided arguments during compile time
    // with generating an appropriate integer tuple with generateIndexes
    // and provide a default constructed instance of that type to
    // instantiate the correct implementation of the actual bind helper
    // function that binds all arguments correctly
    typename ns_mpl::generateIndexes<
      sizeof...(Args), // for each argument in the list
      1                // starting with placeholder index 1
    >::type()
    );
}

} /* end namespace ns_utilities */

#endif /* LIBCPP_UTILITIES_BINDVARIADIC_HPP_ */
