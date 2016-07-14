/*
 * UniqueInstance.hpp
 *
 *  Created on: Jan 27, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_UNIQUEINSTANCE_HPP_
#define LIBCPP_TYPES_UNIQUEINSTANCE_HPP_

#include "UniquePtr.hpp"

#include <type_traits>
#include <boost/core/ignore_unused.hpp>


namespace ns_types {


/**
 * @brief Base class for all types whose instances implement a unique ownership semantic.
 *
 * This class can be used as base for all types @c T where it shall be enforced
 * that instances of this type are only created via unique ownership semantics.
 *
 * The underlying type @c T is passed as template argument for being able to
 * create an instance via a static factory method, using the CRTP.
 *
 * @tparam T the underlying type, allowed to be incomplete
 * @tparam Clonable flag indicating if cloning instances shall be enabled
 */
template<
	typename T,
	bool Clonable = false
>
class UniqueInstance {
protected:

	/**
	 * @brief Internal structure for instance creation.
	 *
	 * This structure is only used internally to enforce the creation of
	 * @c UniqueInstances via its static @c create() factory method.
	 * For using @c make_unique() the constructor must be public.
	 * This structure is injected as additional parameter which is not
	 * visible outside of this class.
	 */
	struct CreationKey {};

	// make sure that the underlying type provides a valid copy constructor taking the
	// internal creation key as first argument for using the clone() method
	static_assert( Clonable == false
			|| std::is_constructible< T, const CreationKey&, const T& >::value == true,
		"type must provide copy constructor with internal creation key for being clonable" );

public:

	/// an instance of @c T with shared ownership semantics
	typedef UniquePtr< T > InstancePtr;
	typedef UniquePtr< const T > ConstInstancePtr;


	/**
	 * Constructor for a new unique instance of type @c T.
	 *
	 * This constructor takes a reference to the internal @c CreationKey as argument
	 * in order to enforce instance creation via the static @c create() factory method.
	 *
	 * @param[in] key
	 *               the internal creation key to enforce the instance creation
	 *               via the @c create() factory method
	 */
	UniqueInstance( CreationKey key ) {
		// suppress compiler warnings since we need a parameter name for doxygen
		boost::ignore_unused( key );
	}

	/**
	 * @brief Create a new shared instance of the underlying type.
	 *
	 * This factory method creates a new @c SharedRef instance and passes its
	 * ownership to the caller.
	 *
	 * @tparam Args
	 *     a variadic list of constructor argument types
	 * @param[in] args
	 *     the argument list for constructing an instance of type T
	 * @return the newly created instance, ownership is transferred to the caller
	 */
	template< typename... Args >
	static InstancePtr create( Args&&... args ) {
		static_assert( std::is_constructible< T, const CreationKey&, Args... >::value == true,
			"type not constructible from the argument list" );
		return std::move( ns_types::make_unique< T >( CreationKey(), std::forward< Args >(args)... ) );
	}

	/**
	 * @brief Create a copy of the @c UniqueInstance.
	 *
	 * If this method is invoked, the underlying type @c T must provide a valid
	 * copy constructor, accepting the internal @c CreationKey as
	 *
	 * @return a new @c UniqueInstance of @c T
	 */
	InstancePtr clone() const {
		static_assert( Clonable == true, "type must be marked clonable" );
		return std::move( ns_types::make_unique< T >( CreationKey(), static_cast< const T& >( *this ) ) );
	}

};

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_UNIQUEINSTANCE_HPP_ */
