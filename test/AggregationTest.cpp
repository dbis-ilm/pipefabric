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

#include <boost/core/ignore_unused.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "core/Tuple.hpp"
#include "qop/AggregateFunctions.hpp"
#include "qop/AggregateStateBase.hpp"
#include "qop/Aggregation.hpp"
#include "qop/SlidingWindow.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr<double> InTuplePtr;
typedef TuplePtr<double, double, int> OutTuplePtr;

typedef TuplePtr<double, double, double, double> Out2TuplePtr;

TEST_CASE( "Compute a simple aggregate on the entire stream", "[Aggregation]" ) {
	typedef Aggregator3<InTuplePtr,
											AggrSum<double>, 0,
											AggrAvg<double, double>, 0,
											AggrCount<double, int>, 0> MyAggrState;
	typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	typedef Aggregation<InTuplePtr, OutTuplePtr, MyAggrState> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(1.0), makeTuplePtr(2.0), makeTuplePtr(3.0),
		makeTuplePtr(4.0), makeTuplePtr(5.0), makeTuplePtr(6.0) };

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr (21.0, 3.5, 6)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);
	auto aggr = std::make_shared<TestAggregation>(
							 MyAggrState::finalize, MyAggrState::iterate, TriggerByCount, 6);

	CREATE_LINK(mockup, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();
	REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

TEST_CASE( "Compute an incremental aggregate on the entire stream", "[Aggregation]" ) {
	typedef Aggregator3<InTuplePtr,
											AggrSum<double>, 0,
											AggrAvg<double, double>, 0,
											AggrCount<double, int>, 0> MyAggrState;
	typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	typedef Aggregation<InTuplePtr, OutTuplePtr, MyAggrState> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(1.0), makeTuplePtr(2.0), makeTuplePtr(3.0),
		makeTuplePtr(4.0), makeTuplePtr(5.0), makeTuplePtr(6.0)
	};

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr(1.0, 1.0, 1),
		makeTuplePtr(3.0, 1.5, 2),
		makeTuplePtr(6.0, 2.0, 3),
		makeTuplePtr(10.0, 2.5, 4),
		makeTuplePtr(15.0, 3.0, 5),
		makeTuplePtr(21.0, 3.5, 6)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);
	auto aggr = std::make_shared<TestAggregation>(
							 MyAggrState::finalize, MyAggrState::iterate);

	CREATE_LINK(mockup, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());

}

TEST_CASE( "Compute an incremental min/maxaggregate on the stream", "[Aggregation]" ) {
	typedef Aggregator4<InTuplePtr,
											AggrMinMax<double, std::less<double>>, 0,
											AggrMinMax<double, std::greater<double>>, 0,
											AggrMRecent<double>, 0,
											AggrLRecent<double>, 0
										 > MyAggrState;
	typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	typedef Aggregation<InTuplePtr, Out2TuplePtr, MyAggrState > TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(3.4), makeTuplePtr(2.1), makeTuplePtr(3.0),
		makeTuplePtr(5.7), makeTuplePtr(9.1), makeTuplePtr(7.4)
	};

	std::vector<Out2TuplePtr> expected = {
		makeTuplePtr(3.4, 3.4, 3.4, 3.4), makeTuplePtr(2.1, 3.4, 2.1, 3.4),
		makeTuplePtr(2.1, 3.4, 3.0, 3.4), makeTuplePtr(2.1, 5.7, 5.7, 3.4),
		makeTuplePtr(2.1, 9.1, 9.1, 3.4), makeTuplePtr(2.1, 9.1, 7.4, 3.4)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, Out2TuplePtr> >(input, expected);
	auto aggr = std::make_shared<TestAggregation>(
		MyAggrState::finalize, MyAggrState::iterate
	);

	CREATE_LINK(mockup, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());

}

TEST_CASE( "Compute an incremental min/maxaggregate on a window", "[Aggregation]" ) {
	typedef Aggregator4<InTuplePtr,
											AggrMinMax<double, std::less<double>>, 0,
											AggrMinMax<double, std::greater<double>>, 0,
											AggrMRecent<double>, 0,
											AggrLRecent<double>, 0
										 > MyAggrState;
	typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	typedef Aggregation<InTuplePtr, Out2TuplePtr, MyAggrState > TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(3.4), makeTuplePtr(2.1), makeTuplePtr(3.0),
		makeTuplePtr(5.7), makeTuplePtr(9.1), makeTuplePtr(7.4)
	};

	std::vector<Out2TuplePtr> expected = {
		makeTuplePtr(3.4, 3.4, 3.4, 3.4),
		makeTuplePtr(2.1, 3.4, 2.1, 3.4),
		makeTuplePtr(2.1, 3.4, 3.0, 3.4),
		makeTuplePtr(2.1, 3.0, 3.0, 2.1), // outdated
		makeTuplePtr(2.1, 5.7, 5.7, 2.1),
		makeTuplePtr(3.0, 5.7, 5.7, 3.0), // outdated
		makeTuplePtr(3.0, 9.1, 9.1, 3.0),
		makeTuplePtr(5.7, 9.1, 9.1, 5.7), // outdated
		makeTuplePtr(5.7, 9.1, 7.4, 5.7)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, Out2TuplePtr> >(input, expected);
	auto win = std::make_shared<SlidingWindow<InTuplePtr>>(WindowParams::RowWindow, 3);
	auto aggr = std::make_shared<TestAggregation>(
		MyAggrState::finalize, MyAggrState::iterate
	);

	CREATE_LINK(mockup, win);
	CREATE_LINK(win, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}
