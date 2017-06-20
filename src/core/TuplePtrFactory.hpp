/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef TuplePtrFactory_hpp_
#define TuplePtrFactory_hpp_

#include "Tuple.hpp"


namespace pfabric {

/**
 * @brief A factory class for generating tuple pointer instances.
 *
 * This class satisfies the @c TupleFactoryTraits and implements a unified factory
 * interface for generating arbitrary tuples that are allocated on the heap and are
 * referenced via intrusive smart pointers for automatic memory management.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
class TuplePtrFactory {
public:

	/**
	 * @brief Meta function returning the result element type of the factory function
	 *        when the @c create() method is invoked with @c Types as arguments.
	 *
	 * @tparam Types
	 *    argument list for constructing a new element instance
	 */
	template< typename... Types >
	struct getElementType {
		typedef TuplePtr< typename std::decay< Types >::type... > type;
	};

	/**
	 * @brief Factory method for creating new element instances.
	 *
	 * Note, that tuples should be always created on the heap and handled by smart pointers
	 * (intrusive pointers). For this purpose, this function is provided,
	 * which creates a Tuple object and returns a smart pointer
	 * to this object.
	 *
	 * @param[in] args
	 *    a list of arguments for creating a new tuple
	 * @return the newly created tuple instance
	 */
	template< typename... Args >
	static typename getElementType< Args... >::type create( Args&&... args ) {
		typedef Tuple< typename std::decay< Args >::type... > ResultTuple;
		return boost::intrusive_ptr< ResultTuple >( new ResultTuple( std::forward< Args >(args)...) );
	}
};


/**
 * @brief Creates a new tuple of the given type and returns an (intrusive) pointer to it.
 *
 * Note, that tuples should be always created on the heap and handled by smart pointers
 * (intrusive pointers). For this purpose, this function is provided,
 * which creates a Tuple object and returns a smart pointer
 * to this object.
 */
template<typename... Types>
auto makeTuplePtr( Types&&... args )
	-> decltype(
		(TuplePtrFactory::create( std::forward< Types >(args)... ))
	)
{
	return TuplePtrFactory::create( std::forward< Types >(args)... );
}


} /* end namespace pfabric */


#endif /* TuplePtrFactory_hpp_ */
