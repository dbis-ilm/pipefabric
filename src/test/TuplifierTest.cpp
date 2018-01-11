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

#include <vector>

#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/Tuplifier.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr<std::string, std::string, std::string> InTuplePtr;
typedef TuplePtr<std::string, std::string, double, std::string> OutTuplePtr;

/**
 * A test of the tuplifier operator using the ordered mode.
 */
TEST_CASE( "Ordered tuplification", "[Tuplifier]" ) {
	typedef Tuplifier<InTuplePtr, OutTuplePtr> TestTuplifier;

	auto tgen = std::make_shared<StreamMockup<InTuplePtr, OutTuplePtr>>("tuplifier_test1.in", "tuplifier_test1.res");

	auto top = std::make_shared<TestTuplifier>(std::initializer_list<std::string>({
      "http://data.org/name", "http://data.org/price", 
      "http://data.org/someOther"}), 
			TuplifierParams::ORDERED);
	CREATE_LINK(tgen, top);
	CREATE_DATA_LINK(top, tgen);

	tgen->start();
}

/**
 * A test of the tuplifier operator using punctuation mode.
 */
TEST_CASE( "Punctuated tuplification", "[Tuplifier]" ) {
	typedef Tuplifier<InTuplePtr, OutTuplePtr> TestTuplifier;

 		auto tgen = std::make_shared<StreamMockup<InTuplePtr, OutTuplePtr>>("tuplifier_test1.in", "tuplifier_test1.res");

		auto top = std::make_shared<Tuplifier<InTuplePtr, OutTuplePtr> >(std::initializer_list<std::string>({
				"http://data.org/name", "http://data.org/price", "http://data.org/someOther"}),
				TuplifierParams::PUNCTUATED);
		CREATE_LINK(tgen, top);
		CREATE_DATA_LINK(top, tgen);
		tgen->start();
}

/**
 * A test of the tuplifier operator using completed mode.
 */
TEST_CASE( "Completed tuplification", "[Tuplifier]" ) {
	typedef Tuplifier<InTuplePtr, OutTuplePtr> TestTuplifier;

 		auto tgen = std::make_shared<StreamMockup<InTuplePtr, OutTuplePtr>>("tuplifier_test1.in", "tuplifier_test1.res");

		auto top = std::make_shared<TestTuplifier>(std::initializer_list<std::string>({
				"http://data.org/name", "http://data.org/price", "http://data.org/someOther"}),
				TuplifierParams::COMPLETED);
		CREATE_LINK(tgen, top);
		CREATE_DATA_LINK(top, tgen);
		tgen->start();
}
