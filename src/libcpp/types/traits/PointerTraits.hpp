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
 * PointerTraits.hpp
 *
 *  Created on: Mar 11, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TRAITS_POINTERTRAITS_HPP_
#define LIBCPP_TYPES_TRAITS_POINTERTRAITS_HPP_

#include <type_traits>
#include <memory>
#include <utility>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/if.hpp>
#include <boost/core/ignore_unused.hpp>


namespace ns_types {

template< typename P >
class PointerTraits {
public:

	typedef std::is_pointer< P > isPointer;

	typedef P PointerType;

	typedef typename boost::mpl::if_<
		isPointer, typename std::remove_pointer< P >::type, void
	>::type ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		return allocateImpl( isPointer(), std::forward< Args >( args )... );
	}

	static void destroy( PointerType& p ) {
		destroyImpl( isPointer(), p );
	}

	static bool isNull( const PointerType& p ) {
		return isNullImpl( isPointer(), p );
	}

private:

	template< typename... Args >
	static PointerType allocateImpl( const std::true_type& isPointer, Args&&... args ) {
		boost::ignore_unused( isPointer );
		return new ElementType( std::forward< Args >( args )... );
	}

	template< typename... Args >
	static PointerType allocateImpl( const std::false_type& isPointer, Args&&... args ) {
		boost::ignore_unused( isPointer );
		return ElementType( std::forward< Args >( args )... );
	}


	static void destroyImpl( const std::true_type& isPointer, PointerType& p ) {
		boost::ignore_unused( isPointer );
		delete p;
		p = nullptr;
	}

	static void destroyImpl( const std::false_type& isPointer, PointerType& p ) {
		boost::ignore_unused( isPointer );
		boost::ignore_unused( p );
	}

	static bool isNullImpl( const std::true_type& isPointer, const PointerType& p ) {
		return p == nullptr;
	}

	static bool isNullImpl( const std::false_type& isPointer, const PointerType& p ) {
		boost::ignore_unused( p );
		return false;
	}
};


///////   boost pointer types   ///////


template< typename T >
class PointerTraits< boost::scoped_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef boost::scoped_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		return PointerType( new ElementType( std::forward< Args >( args )... ) );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}

	static bool isNull( const PointerType& p ) {
		return p.get() == nullptr;
	}
};

template< typename T >
class PointerTraits< boost::shared_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef boost::shared_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		return boost::make_shared< ElementType >( std::forward< Args >( args )... );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}

	static bool isNull( const PointerType& p ) {
		return p.get() == nullptr;
	}
};

template< typename T >
class PointerTraits< boost::weak_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef boost::weak_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		// weak pointers do not allocate anything - they can only be copied or created from shared pointers
		return PointerType( std::forward< Args >( args )... );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}

	static bool isNull( const PointerType& p ) {
		return p.get() == nullptr;
	}
};

template< typename T >
class PointerTraits< boost::intrusive_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef boost::intrusive_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		return PointerType( new ElementType( std::forward< Args >( args )... ) );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}

	static bool isNull( const PointerType& p ) {
		return p.get() == nullptr;
	}
};



///////   std pointer types   ///////

template< typename T >
class PointerTraits< std::unique_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef std::unique_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		return std::move( PointerType( new ElementType( std::forward< Args >( args )... ) ) );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}
};

template< typename T >
class PointerTraits< std::shared_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef std::shared_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		return std::make_shared< ElementType >( std::forward< Args >( args )... );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}

	static bool isNull( const PointerType& p ) {
		return p.get() == nullptr;
	}
};

template< typename T >
class PointerTraits< std::weak_ptr< T > > {
public:

	typedef std::true_type isPointer;

	typedef boost::weak_ptr< T > PointerType;
	typedef T ElementType;

	template< typename... Args >
	static PointerType allocate( Args&&... args ) {
		// weak pointers do not allocate anything - they can only be copied or created from shared pointers
		return PointerType( std::forward< Args >( args )... );
	}

	static void destroy( PointerType& p ) {
		p.reset();
	}

	static bool isNull( const PointerType& p ) {
		return p.get() == nullptr;
	}
};


template< typename P >
class PointerTraits< const P > : public PointerTraits< P >
{};

template< typename P >
class PointerTraits< P& > : public PointerTraits< P >
{};

template< typename P >
class PointerTraits< P&& > : public PointerTraits< P >
{};


///////   global accessor functions   ///////

template<
	typename PointerType,
	typename... Args
>
PointerType allocatePointer( Args&&... args ) {
	return PointerTraits< PointerType >::allocate( std::forward< Args >( args )... );
}


template<
	typename PointerType
>
void destroyPointer( PointerType& p ) {
	PointerTraits< PointerType >::destroy( p );
}

template<
	typename PointerType
>
bool isNullPointer( const PointerType& p ) {
	return PointerTraits< PointerType >::isNull( p );
}


} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_TRAITS_POINTERTRAITS_HPP_ */
