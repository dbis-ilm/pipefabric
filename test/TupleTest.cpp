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

#include <boost/assign/std/vector.hpp>
#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"

#ifdef HAS_EIGEN
#include <Eigen/Dense>
#endif

using namespace boost::assign;
using namespace pfabric;
using namespace ns_types;

/**
 *
 * Test cases for the ptuple template.
 */

/**
 * Test #1: Create a tuple from data.
 */
TEST_CASE( "Tuple creation", "[Tuple]" ) {

	typedef TuplePtr<int, int, long, std::string, double, boost::posix_time::ptime> TheTuplePtr;

	boost::posix_time::ptime tm (boost::posix_time::microsec_clock::local_time());

	auto tup = makeTuplePtr(1, 2, 3L, std::string("9"), 4.5678, tm);
	REQUIRE(tup->getAttribute<0>() == 1);
	REQUIRE(tup->getAttribute<1>() == 2);
	REQUIRE(tup->getAttribute<2>() == 3L);
	REQUIRE(tup->getAttribute<3>() == std::string("9"));
	REQUIRE(tup->getAttribute<4>() == 4.5678);
	REQUIRE(tup->getAttribute<5>() == tm);
	int i = 2;
	long l = boost::get<long>(dynamic_get(i, *tup));
	REQUIRE(l == 3L);
}

/**
 * Test #2: Create a tuple from a string array.
 */
TEST_CASE( "Tuple creation from string", "[Tuple]" ) {
	typedef TuplePtr<int, int, long, std::string, double, boost::posix_time::ptime> TheTuplePtr;

	boost::posix_time::ptime tm (boost::posix_time::microsec_clock::local_time());
	StringTuple data = { "1", "2", "3", "Nine", "4.5678", boost::posix_time::to_iso_string(tm) };
	TheTuplePtr tup (new TheTuplePtr::element_type(data));
	REQUIRE(tup->getAttribute<0>() == 1);
	REQUIRE(tup->getAttribute<1>() == 2);
	REQUIRE(tup->getAttribute<2>() == 3L);
	REQUIRE(tup->getAttribute<3>() == std::string("Nine"));
	REQUIRE(tup->getAttribute<4>() == 4.5678);
	REQUIRE(tup->getAttribute<5>() == tm);
}

/**
 * Test #3: Create some tuples and check comparison operators.
 */
TEST_CASE( "Tuple comparison", "[Tuple]" ) {
	typedef TuplePtr<int, int, long, std::string, double> TheTuplePtr;

	TheTuplePtr t1 = makeTuplePtr(1, 2, 3L, std::string("9"), 4.5678);
	TheTuplePtr t2 = makeTuplePtr(1, 2, 3L, std::string("9"), 4.5678);
	TheTuplePtr t3 = makeTuplePtr(1, 3, 3L, std::string("9"), 4.5678);
	TheTuplePtr t4 = makeTuplePtr(1, 3, 1L, std::string("9"), 2.5678);

	REQUIRE(*t1 == *t1);
	REQUIRE(*t1 == *t2);
	REQUIRE(!(*t1 == *t3));
	REQUIRE(!(*t1 < *t2));
	REQUIRE(*t1 < *t3);
	REQUIRE(*t1 < *t4);
}

/**
 * Test #4: Create a tuple, serialize it to a buffer, and try to deserialize it again.
 */
TEST_CASE( "Tuple serialization", "[Tuple]" ) {
	typedef TuplePtr<int, std::string, double> TheTuplePtr;

	StreamType res;

	TheTuplePtr tp1 = makeTuplePtr(12, std::string("Hallo"), 42.0);
	tp1->serializeToStream(res);
	TheTuplePtr tp2 = makeTuplePtr(0, std::string(""), 0.0);
	tp2->deserializeFromStream(res);
	REQUIRE(*tp1 == *tp2);
}


/**
 * Test #5: Check handling of null values.
 */
TEST_CASE("Tuple null fields", "[Tuple]") {
	typedef TuplePtr<int, int, long, std::string, double> TheTuplePtr;

	TheTuplePtr tp = makeTuplePtr(1, 2, 3L, std::string("9"), 4.5678);
	REQUIRE(! tp->isNull(0));
	REQUIRE(! tp->isNull(1));
	REQUIRE(! tp->isNull(2));
	REQUIRE(! tp->isNull(3));
	REQUIRE(! tp->isNull(4));
	tp->setNull(1);
	tp->setNull(3);
	REQUIRE(! tp->isNull(0));
	REQUIRE(tp->isNull(1));
	REQUIRE(! tp->isNull(2));
	REQUIRE(tp->isNull(3));
	REQUIRE(! tp->isNull(4));
}

TEST_CASE("Tuple microbenchmarking", "[Tuple]") {
	StreamType res;
	{
		auto tpp1 = makeTuplePtr(1, 345, std::string("Hallo"), 5.6789);
		auto tpp2 = makeTuplePtr(0, 0, std::string(""), 0.0);
		res.clear();
		auto t1 = std::chrono::high_resolution_clock::now();
		for (unsigned int i = 0; i < 100000; i++) {
			tpp1->serializeToStream(res);
			tpp2->deserializeFromStream(res);
			res.clear();
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 100000.0
							<< " microseconds\n";
	}
	{
		auto tpp = makeTuplePtr(1, 345, std::string("Hallo"), 5.6789);
		auto t1 = std::chrono::high_resolution_clock::now();
		for (unsigned int i = 0; i < 1000000; i++) {
			auto d = tpp->getAttribute<1>();
			boost::ignore_unused( d );
			auto f = tpp->getAttribute<3>();
			boost::ignore_unused( f );
			auto s = tpp->getAttribute<2>();
			boost::ignore_unused( s );
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 3000000.0
							<< " microseconds\n";

	}
}

#ifdef HAS_EIGEN

BOOST_AUTO_TEST_CASE(MatrixTest)
{
	typedef TuplePtr<int, std::string, Eigen::MatrixXd> MTuplePtr;

	Eigen::MatrixXd m = Eigen::MatrixXd::Constant(3, 3, 1.0);
	auto tup = makeTuplePtr(1, std::string("Matrix"), m);
	get<2>(*tup)(0, 0) = 1.5;
	get<2>(*tup)(1, 1) = 2.5;
	get<2>(*tup)(2, 2) = 3.5;

	BOOST_CHECK_EQUAL(get<2>(*tup)(0, 0), 1.5);
	BOOST_CHECK_EQUAL(get<2>(*tup)(1, 1), 2.5);
	BOOST_CHECK_EQUAL(get<2>(*tup)(2, 2), 3.5);
	BOOST_CHECK_EQUAL(get<2>(*tup)(0, 1), 1.0);
	BOOST_CHECK_EQUAL(get<0>(*tup), 1);
	BOOST_CHECK_EQUAL(get<1>(*tup), std::string("Matrix"));

	get<2>(*tup) = Eigen::MatrixXd::Constant(3, 3, 10.0);
	BOOST_CHECK_EQUAL(get<2>(*tup)(0, 0), 10.0);
	BOOST_CHECK_EQUAL(get<2>(*tup)(1, 1), 10.0);
	BOOST_CHECK_EQUAL(get<2>(*tup)(2, 2), 10.0);
}

#endif
