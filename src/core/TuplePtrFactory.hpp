/*
 * TuplePtrFactory.hpp
 *
 *  Created on: Mar 15, 2015
 *      Author: fbeier
 */

#ifndef TUPLEPTRFACTORY_HPP_
#define TUPLEPTRFACTORY_HPP_

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
		typedef TuplePtr< Tuple< typename std::decay< Types >::type... > > type;
	};

	/**
	 * @brief Factory method for creating new element instances.
	 *
	 * Note, that tuples should be always created on the heap and handled by smart pointers
	 * (intrusive pointers). For this purpose, this function is provided,
	 * which creates a Tuple object with current time as timestamp and returns a smart pointer
	 * to this object.
	 *
	 * @param[in] args
	 *    a list of arguments for creating a new tuple
	 * @return the newly created tuple instance
	 */
	template< typename... Args >
	static typename getElementType< Args... >::type create( Args&&... args ) {
		typedef Tuple< typename std::decay< Args >::type... > ResultTuple;
		return TuplePtr< ResultTuple >( new ResultTuple( std::forward< Args >(args)...) );
	}
};


/**
 * @brief Creates a new tuple of the given type and returns an (intrusive) pointer to it.
 *
 * Note, that tuples should be always created on the heap and handled by smart pointers
 * (intrusive pointers). For this purpose, this function is provided,
 * which creates a Tuple object with current time as timestamp and returns a smart pointer
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


} /* end namespace pquery */


#endif /* TUPLEPTRFACTORY_HPP_ */
