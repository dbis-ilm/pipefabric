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
 * SharedInstance.hpp
 *
 *  Created on: Jan 27, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_SHAREDINSTANCE_HPP_
#define LIBCPP_TYPES_SHAREDINSTANCE_HPP_

#include "SharedPtr.hpp"

#include <type_traits>
#include <boost/noncopyable.hpp>
#include <boost/core/ignore_unused.hpp>


namespace ns_types {


/**
 * @brief Base class for all types whose instances implement a shared ownership semantic.
 *
 * This class can be used as base for all types @c T where it shall be enforced
 * that instances of this type are only created via shared ownership semantics.
 *
 * The underlying type @c T is passed as template argument for being able to
 * create an instance via a static factory method, using the CRTP.
 * If the @c Clonable flag is set, instances that have been created via
 * the factory method can be cloned, i.e., the source instance is deep-copied
 * via invoking @T's copy constructor.
 *
 * @tparam T the underlying type, allowed to be incomplete
 * @tparam Clonable flag indicating if cloning instances shall be enabled
 */
template<
	typename T,
	bool Clonable = false
>
class SharedInstance :
	public EnableSharedFromThis< T >,
	// hide copy construction and assignment for enforcing usage of clone()
	// to create shared a copy of an instance
	private boost::noncopyable
{
protected:

	/**
	 * @brief Internal structure for instance creation.
	 *
	 * This structure is only used internally to enforce the creation of
	 * @c SharedInstances via its static @c create() factory method.
	 * For using @c make_shared() the constructor must be public.
	 * This structure is injected as additional parameter which is not
	 * visible outside of this class.
	 */
	struct CreationKey {};


public:

	/// an instance of @c T with shared ownership semantics
	typedef SharedPtr< T > SharedRef;
	typedef SharedPtr< const T > SharedConstRef;
	typedef SharedRef Instance;
	typedef SharedConstRef ConstInstance;

	/// a weak shared instance of @c T
	typedef WeakPtr< T > WeakRef;
	typedef WeakPtr< const T > WeakConstRef;


	/**
	 * Constructor for a new shared instance of type @c T.
	 *
	 * This constructor takes a reference to the internal @c CreationKey as argument
	 * in order to enforce instance creation via the static @c create() factory method.
	 *
	 * @param[in] key
	 *               the internal creation key to enforce the instance creation
	 *               via the @c create() factory method
	 */
	SharedInstance( const CreationKey& key ) {
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
	static Instance create( Args&&... args ) {
		static_assert( std::is_constructible< T, const CreationKey&, Args... >::value == true,
			"type not constructible from the argument list" );
		return ns_types::make_shared< T >( CreationKey(), std::forward< Args >(args)... );
	}


	/**
	 * @brief Create a copy of the @c SharedInstance.
	 *
	 * If this method is invoked, the underlying type @c T must provide a valid
	 * copy constructor, accepting the internal @c CreationKey as
	 *
	 * @return a new @c SharedInstance of @c T
	 */
	Instance clone() const {
		static_assert( Clonable == true, "type must be marked clonable" );
		return ns_types::make_shared< T >( CreationKey(), static_cast< const T& >( *this ) );
	}


	/**
	 * @brief Get a reference to the shared @c InputChannel instance.
	 * @return the shared reference to this
	 */
	inline SharedRef getSharedRef() {
		return this->shared_from_this();
	}

	/**
	 * @brief Get a const reference to the shared @c InputChannel instance.
	 * @return the shared const reference to this
	 */
	inline SharedConstRef getSharedRef() const {
		return this->shared_from_this();
	}

	/**
	 * @brief Get a weak reference to the shared @c InputChannel instance.
	 * @return the weak reference to this
	 */
	inline WeakRef getWeakRef() {
		return WeakRef( this->shared_from_this() );
	}

	/**
	 * @brief Get a weak const reference to the shared @c InputChannel instance.
	 * @return the weak const reference to this
	 */
	inline WeakConstRef getWeakRef() const {
		return WeakConstRef( this->shared_from_this() );
	}
};

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_SHAREDINSTANCE_HPP_ */
