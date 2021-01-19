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

#include "catch.hpp"

#include "pubsub/signals/BoostSignal.hpp"
#include "pubsub/signals/OneToManySignal.hpp"
#include "pubsub/signals/OneToOneSignal.hpp"
#include "pubsub/signals/BoostSlot.hpp"
#include "pubsub/signals/StdSlot.hpp"

#include "libcpp/mpl/algorithms/StaticForEach.hpp"

#include <boost/mpl/vector.hpp>



/**
 * @brief Helper structure for checking multiple callback types.
 */
class CallResults {
public:

	CallResults() :
		mFunctorInvoked( false ),
		mGlobalFunctionInvoked( false ),
		mLambdaInvoked( false ) {
	}

	void reset() {
		mFunctorInvoked = false;
		mGlobalFunctionInvoked = false;
		mLambdaInvoked = false;
	}

	void setFunctorInvoked() {
		mFunctorInvoked = true;
	}

	bool functorInvoked() const {
		return mFunctorInvoked;
	}

	void setGlobalFunctionInvoked() {
		mGlobalFunctionInvoked = true;
	}

	bool globalFunctionInvoked() const {
		return mGlobalFunctionInvoked;
	}

	void setLambdaInvoked() {
		mLambdaInvoked = true;
	}

	bool lambdaInvoked() const {
		return mLambdaInvoked;
	}

private:

	bool mFunctorInvoked;
	bool mGlobalFunctionInvoked;
	bool mLambdaInvoked;
};

class Int {
public:

	Int( Int&& other ) = default;
	Int( const Int& other ) = default;
	Int( int i ) : mI(i) {}

	Int& operator=( const Int& other ) = default;
	Int& operator=( Int&& other ) = default;

	int getInt() const { return mI; }

private:
	int mI;
};


static CallResults testResults; /**< global structure for callback results */
static const int testInt = 1;   /**< some predefined test data */
static const Int testInteger = Int( testInt );


void globalFunction( const int i, const Int I ) {
	REQUIRE( i == testInt );
	REQUIRE( I.getInt() == testInt );
	testResults.setGlobalFunctionInvoked();
}

struct Functor {
	void operator()( const int i, Int I ) {
		REQUIRE( i == testInt );
		REQUIRE( I.getInt() == testInt );
		testResults.setFunctorInvoked();
	}
};

auto lambdaFunction = []( const int i, const Int& I ){
	REQUIRE( i == testInt );
	REQUIRE( I.getInt() == testInt );
	testResults.setLambdaInvoked();
};


/**
 * @brief Meta function class for testing signal-slot implementations.
 */
struct SignalTest {

	/**
	 * @brief Execute a single test case for one signal-slot implementation.
	 *
	 * @tparam Signal
	 *    the signal-slot implementation type that shall be tested
	 */
	//template< typename TestSignalPtr >
	template< typename Signal >
	static void apply() {

		// common interface for all signal implementations
		typedef SignalTraits< Signal > SignalInterface;

		{ // test global function
			// create the signal and connect a test slot
			Signal signal;
			auto connection = SignalInterface::connect( signal, std::bind(&globalFunction, std::placeholders::_1, std::placeholders::_2 ) );

			// invoke the signal
			testResults.reset();

			// verify the results
			SignalInterface::publish( signal, testInt, testInteger );
			REQUIRE( testResults.globalFunctionInvoked() );
			REQUIRE( !testResults.functorInvoked() );
			REQUIRE( !testResults.lambdaInvoked() );

			// disconnect the slot
			SignalInterface::disconnect( signal, std::move( connection ) );
		}

		{ // test functor
			// create the signal and connect a test slot
			Signal signal;
			auto connection = SignalInterface::connect( signal, Functor() );

			// invoke the signal
			testResults.reset();

			// verify the results
			SignalInterface::publish( signal, testInt, testInteger );
			REQUIRE( !testResults.globalFunctionInvoked() );
			REQUIRE( testResults.functorInvoked() );
			REQUIRE( !testResults.lambdaInvoked() );

			// disconnect the slot
			SignalInterface::disconnect( signal, std::move( connection ) );
		}

		{ // test lambda function
			// create the signal and connect a test slot
			Signal signal;
			auto connection = SignalInterface::connect( signal, lambdaFunction );

			// invoke the signal
			testResults.reset();

			// verify the results
			SignalInterface::publish( signal, testInt, testInteger );
			REQUIRE( !testResults.globalFunctionInvoked() );
			REQUIRE( !testResults.functorInvoked() );
			REQUIRE( testResults.lambdaInvoked() );

			// disconnect the slot
			SignalInterface::disconnect( signal, std::move( connection ) );
		}
	}
};

TEST_CASE("Verifying  different sihnal implementations.", "[Signal]") {
	using namespace boost::mpl;
	using namespace ns_mpl;

	///////// compile time

	// all signal implementations that shall be compared
	// (cross product problem here due to template template parameter)
	typedef typename vector<
		// need to add pointer since mpl::for_each requires an instance to be created
		OneToOneSignal< StdSlot, int, Int >,
		OneToOneSignal< BoostSlot, int, Int >,
		OneToManySignal< StdSlot, int, Int >,
		OneToManySignal< BoostSlot, int, Int >,
		BoostSignal< StdSlot, int, Int >,
		BoostSignal< BoostSlot, int, Int >
	>::type TestSignals;


	///////// runtime

	// run the benchmark for each compile time parameter
	staticForEach< TestSignals, SignalTest >();
}
