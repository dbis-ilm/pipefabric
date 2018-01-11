/*
 * Copyright (c) 2014-18 The PipeFabric team,
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

 #include "catch.hpp"

#include <boost/mpl/assert.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/vector.hpp>
#include <string>

#include "core/StreamElementTraits.hpp"
#include "core/Tuple.hpp"
#include "core/PFabricTypes.hpp"
#include "libcpp/mpl/algorithms/StaticForEach.hpp"


using namespace pfabric;
using namespace boost::mpl;


/**
 * @brief Custom element type satisfying the @c StreamElementTraits.
 */
struct CustomElement {
	static const TupleSize NUM_ATTRIBUTES = 3;
	typedef std::tuple< int&, char&, int& > Values;

	CustomElement( const int a, const char b, const int c ) :
		v0( a ), v1( b ), v2( c ), values( std::tie( v0, v1, v2 ) ) {
		for( unsigned i = 0; i < NUM_ATTRIBUTES; ++i ) {
			nulls[ i ] = false;
		}
	};

	template< AttributeIdx ID >
	struct getAttributeType {
		typedef typename std::remove_reference<
			typename std::tuple_element< ID, Values >::type
		>::type type;
	};

	template< AttributeIdx ID >
	typename getAttributeType< ID >::type& getAttribute() {
		return std::get< ID >( values );
	}

	template< AttributeIdx ID >
	const typename getAttributeType< ID >::type& getAttribute() const {
		return std::get< ID >( values );
	}

	template< AttributeIdx ID, typename Value >
	void setAttribute( const Value& value ) {
		std::get< ID >( values ) = value;
	}


	bool isNull( const AttributeIdx& index ) const { return nulls[ index ]; }
	void setNull( const AttributeIdx& index, const bool value = true ) { nulls[ index ] = value; }
	void setNull() {
		for( unsigned i = 0; i < NUM_ATTRIBUTES; ++i ) {
			nulls[ i ] = true;
		}
	}

	int v0;
	char v1;
	int v2;
	Values values;
	bool nulls[ NUM_ATTRIBUTES ];
};


struct TestStreamElementTraits {

	template< typename ElementType >
	static void apply() {
		typedef StreamElementTraits< ElementType > ElementTraits;

		// check number of attributes
		BOOST_MPL_ASSERT(( equal< typename ElementTraits::StreamElement, ElementType > ));
		REQUIRE( (ElementTraits::NUM_ATTRIBUTES == 3) );
		REQUIRE( (ElementTraits::getNumAttributes() == 3) );

		static_assert( std::is_same< typename ElementTraits::template getAttributeType< 0 >::type, int >::value,
			"attribute 0 is expected to be an int"
		);

		static_assert( std::is_same< typename ElementTraits::template getAttributeType< 1 >::type, char >::value,
			"attribute 0 is expected to be a char"
		);

		static_assert( std::is_same< typename ElementTraits::template getAttributeType< 2 >::type, int >::value,
			"attribute 2 is expected to be an int"
		);

		// check element creation
		auto element = ElementTraits::create( 1, 'a', 2 );
		REQUIRE( ElementTraits::template getAttribute< 0 >( element ) == 1 );
		REQUIRE( getAttribute< 1 >( element ) == 'a' );
		REQUIRE( getAttribute< 2 >( element ) == 2 );

		// check attribute modification
		// via traits
		ElementTraits::template setAttribute< 0 >( element, 10 );
		REQUIRE( getAttribute< 0 >( element ) == 10 );
		REQUIRE( getAttribute< 1 >( element ) == 'a' );
		REQUIRE( getAttribute< 2 >( element ) == 2 );

		// via global accessor functions
		setAttribute< 1 >( element, 'b' );
		REQUIRE( getAttribute< 0 >( element ) == 10 );
		REQUIRE( getAttribute< 1 >( element ) == 'b' );
		REQUIRE( getAttribute< 2 >( element ) == 2 );

			// check null properties
		REQUIRE( !ElementTraits::isNull( element, 0 ) );
		REQUIRE( !ElementTraits::isNull( element, 1 ) );
		REQUIRE( !ElementTraits::isNull( element, 2 ) );

		ElementTraits::setNull( element, 1 );
		REQUIRE( !ElementTraits::isNull( element, 0 ) );
		REQUIRE( ElementTraits::isNull( element, 1 ) );
		REQUIRE( !ElementTraits::isNull( element, 2 ) );

		ElementTraits::setNull( element );
		REQUIRE( ElementTraits::isNull( element, 0 ) );
		REQUIRE( ElementTraits::isNull( element, 1 ) );
		REQUIRE( ElementTraits::isNull( element, 2 ) );

		if( ns_types::PointerTraits< ElementType >::isPointer::value ) {
			ns_types::destroyPointer( element );
		}
	}

};


TEST_CASE( "Properties of StreamElementTraits", "[StreamElementTraits]" ) {
	typedef TuplePtr< int, char, int > TestTuplePtr;
	typedef TestTuplePtr::element_type* RawTestTuplePtr;

	typedef boost::mpl::vector<
		CustomElement,
		TestTuplePtr::element_type,
		TestTuplePtr,
		RawTestTuplePtr
	> TestElementTypes;

	ns_mpl::staticForEach< TestElementTypes, TestStreamElementTraits >();
}
