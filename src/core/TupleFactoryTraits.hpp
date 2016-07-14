/*
 * TupleFactoryTraits.hpp
 *
 *  Created on: Mar 15, 2015
 *      Author: fbeier
 */

#ifndef TUPLEFACTORYTRAITS_HPP_
#define TUPLEFACTORYTRAITS_HPP_


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


#endif /* TUPLEFACTORYTRAITS_HPP_ */
