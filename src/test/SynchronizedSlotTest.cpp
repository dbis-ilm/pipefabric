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

#define BOOST_TEST_MODULE SynchronizedSlotTest
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>

#include "pubsub/signals/SynchronizedSlot.hpp"
#include "pubsub/signals/BoostSlot.hpp"
#include "pubsub/signals/StdSlot.hpp"

#include <boost/bind.hpp>
#include <functional>


typedef SynchronizedSlot< StdSlot< int > > SSlot;
typedef SynchronizedSlot< BoostSlot< const int& > > BSlot;

static const int testData = 42;

void testFunc( int i ) {
	BOOST_CHECK( i == testData );
}


BOOST_AUTO_TEST_CASE( testIsSynchronizedSlot ) {

	BOOST_CHECK( impl::isSynchronizedSlot< StdSlot< int& > >::value == false );
	BOOST_CHECK( impl::isSynchronizedSlot< BoostSlot< const int& > >::value == false );
	BOOST_CHECK( impl::isSynchronizedSlot< SSlot >::value == true );
	BOOST_CHECK( impl::isSynchronizedSlot< BSlot >::value == true );
}


BOOST_AUTO_TEST_CASE( testDefaultConstruction ) {
	BOOST_CHECK_NO_THROW( SSlot defaultSSlot );
	BOOST_CHECK_NO_THROW( BSlot defaultBSlot );
}


BOOST_AUTO_TEST_CASE( testConversionConstruction ) {
	SSlot defaultSSlot;
	BSlot defaultBSlot;
	BOOST_CHECK_NO_THROW( SSlot convertedSSlot = defaultBSlot );
	BOOST_CHECK_NO_THROW( BSlot convertedBSlot = defaultSSlot );
}


BOOST_AUTO_TEST_CASE( testCopyConstruction ) {
	SSlot defaultSSlot;
	BSlot defaultBSlot;
	BOOST_CHECK_NO_THROW( SSlot copiedSSlot = defaultSSlot );
	BOOST_CHECK_NO_THROW( BSlot copuedBSlot = defaultBSlot );
}


BOOST_AUTO_TEST_CASE( testCopyAssignment ) {
	SSlot defaultSSlot;
	BSlot defaultBSlot;
	SSlot convertedSSlot;
	BSlot convertedBSlot;
	BOOST_CHECK_NO_THROW( convertedSSlot = defaultSSlot );
	BOOST_CHECK_NO_THROW( convertedBSlot = defaultBSlot );
}


BOOST_AUTO_TEST_CASE( testConversionAssignment ) {
	SSlot defaultSSlot;
	BSlot defaultBSlot;
	SSlot convertedSSlot;
	BSlot convertedBSlot;
	BOOST_CHECK_NO_THROW( convertedSSlot = defaultBSlot );
	BOOST_CHECK_NO_THROW( convertedBSlot = defaultSSlot );
}


BOOST_AUTO_TEST_CASE( testWrappingConstruction ) {
	BOOST_CHECK_NO_THROW( SSlot wrappedSSlotBoostBind = boost::bind( &testFunc, _1 ) );
	BOOST_CHECK_NO_THROW( BSlot wrappedBSlotBoostBind = boost::bind( &testFunc, _1 ) );
	BOOST_CHECK_NO_THROW( SSlot wrappedSSlotStdBind = std::bind( &testFunc, std::placeholders::_1 ) );
	BOOST_CHECK_NO_THROW( BSlot wrappedBSlotStdBind = std::bind( &testFunc, std::placeholders::_1 ) );
}


BOOST_AUTO_TEST_CASE( testWrappingAssignment ) {
	SSlot wrappedSSlotBoostBind;
	BSlot wrappedBSlotBoostBind;
	SSlot wrappedSSlotStdBind;
	BSlot wrappedBSlotStdBind;
	BOOST_CHECK_NO_THROW( wrappedSSlotBoostBind = boost::bind( &testFunc, _1 ) );
	BOOST_CHECK_NO_THROW( wrappedBSlotBoostBind = boost::bind( &testFunc, _1 ) );
	BOOST_CHECK_NO_THROW( wrappedSSlotStdBind = std::bind( &testFunc, std::placeholders::_1 ) );
	BOOST_CHECK_NO_THROW( wrappedBSlotStdBind = std::bind( &testFunc, std::placeholders::_1 ) );
}


BOOST_AUTO_TEST_CASE( testEmptySlotInvocation ) {
	SSlot defaultSSlot;
	BSlot defaultBSlot;
	BOOST_CHECK_THROW( defaultSSlot( testData ), std::exception );
	BOOST_CHECK_THROW( defaultBSlot( testData ), std::exception );
}


BOOST_AUTO_TEST_CASE( testNonEmptySlotInvocation ) {
	SSlot wrappedSSlotBoostBind = boost::bind( &testFunc, _1 );
	BSlot wrappedBSlotBoostBind = boost::bind( &testFunc, _1 );
	SSlot wrappedSSlotStdBind = std::bind( &testFunc, std::placeholders::_1 );
	BSlot wrappedBSlotStdBind = std::bind( &testFunc, std::placeholders::_1 );
	BOOST_CHECK_NO_THROW( wrappedSSlotBoostBind( testData ) );
	BOOST_CHECK_NO_THROW( wrappedBSlotBoostBind( testData ) );
	BOOST_CHECK_NO_THROW( wrappedSSlotStdBind( testData ) );
	BOOST_CHECK_NO_THROW( wrappedBSlotStdBind( testData ) );
}

