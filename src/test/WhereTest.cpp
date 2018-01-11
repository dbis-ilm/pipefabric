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

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Where.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, int, int > MyTuplePtr;

/**
 * A simple test of the filter operator.
 */
TEST_CASE("Applying a filter to a tuple stream", "[Where]") {
	std::vector<MyTuplePtr> input = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr (1, 1, 10),
		makeTuplePtr(2, 2, 20) };

		std::vector<MyTuplePtr> expected = {
			makeTuplePtr (1, 1, 10),
			makeTuplePtr(2, 2, 20) };

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);

	auto filter_fun = [&]( const MyTuplePtr& tp, bool outdated ) -> bool {
		return tp->getAttribute<2>() > 0;
	};
	auto wop = std::make_shared< Where<MyTuplePtr> >(filter_fun);

	CREATE_DATA_LINK(mockup, wop)
	CREATE_DATA_LINK(wop, mockup)

	mockup->start();
  
  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}
