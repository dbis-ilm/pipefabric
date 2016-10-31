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

#ifndef TupleFactoryTraits_hpp_
#define TupleFactoryTraits_hpp_

#include <utility>

namespace pfabric {

/**
 * @brief Common interface for tuple generator classes.
 *
 * This class implements a unified factory interface for generating arbitrary tuple types.
 * A factory class is required to configure tuple creation policies in a generic
 * way for stream processing operators. All operators that need to generate new
 * elements as output instead of forwarding input elements can be configured with
 * such a @c TupleFactoryImpl class to grant control about memory management
 * on a per-operator base.
 *
 * @tparam TupleFactoryImpl
 *    the TupleFactory implementation for which the traits are defined
 *
 * Specialize the class for all TupleFactory implementations that do not implement the
 * default interface.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename TupleFactoryImpl
>
class TupleFactoryTraits {
private:

	/// the TupleFactory component whose traits are defined here
	typedef TupleFactoryImpl TupleFactory;

public:
	//////   public types   //////

	/**
	 * @brief Meta function returning the result element type of the factory function
	 *        when the @c create() method is invoked with @c Types as arguments.
	 *
	 * @tparam Types
	 *    argument list for constructing a new element instance
	 */
	template< typename... Types >
	struct getElementType {
		typedef typename TupleFactory::template getElementType< Types... >::type type;
	};

	//////   public constants   //////


	//////   public interface   //////

	/**
	 * @brief Factory method for creating new tuple instances.
	 *
	 * This factory method will create new tuple instances using the underlying
	 * @c TupleFactory implementation for which the traits are defined here.
	 *
	 * @tparam Args
	 *    a list of argument types used to create the new tuple instance
	 * @param[in] args
	 *    a list of arguments for creating a new tuple instance
	 * @return a new tuple instance
	 */
	template< typename... Args >
	static typename getElementType< Args... >::type create( Args&&... args ) {
		return TupleFactory::create( std::forward< Args >(args)... );
	}
};


} /* end namespace pfabric */


#endif /* TupleFactoryTraits_hpp_ */
