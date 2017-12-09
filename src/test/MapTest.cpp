#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Map.hpp"
#include "qop/StatefulMap.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, int, int > InTuplePtr;

typedef TuplePtr< int, int, int, int > OutTuplePtr;

/**
 * A simple test of the projection operator.
 */
TEST_CASE("Applying a map function to a tuple stream", "[Map]") {
	typedef Map< InTuplePtr, OutTuplePtr > TestMap;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr (1, 1, 10),
		makeTuplePtr(2, 2, 20) };

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr(0, 0, 0, 0),
		makeTuplePtr (1, 10, 1, 11),
		makeTuplePtr(2, 20, 2, 22) };

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);

	auto map_fun = [&]( const InTuplePtr& tp, bool ) -> OutTuplePtr {
		return makeTuplePtr(
			tp->getAttribute<0>(), tp->getAttribute<2>(), tp->getAttribute<1>(),
			tp->getAttribute<1>() + tp->getAttribute<2>()
		);
	};
	auto mop = std::make_shared< TestMap >(map_fun);

	CREATE_DATA_LINK(mockup, mop)
	CREATE_DATA_LINK(mop, mockup)

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

/**
 * A simple test of the stateful map operator.
 */

struct MyState {
	MyState() : cnt(0), sum(0) {}
	int cnt, sum;
};

TEST_CASE("Applying a stateful map function to a tuple stream", "[StatefulMap]") {
	typedef StatefulMap< InTuplePtr, OutTuplePtr, MyState > TestMap;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr (1, 1, 10),
		makeTuplePtr(2, 2, 20) };

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr(0, 0, 1, 0),
		makeTuplePtr (1, 10, 2, 10),
		makeTuplePtr(2, 20, 3, 30) };

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);

	auto map_fun = [&]( const InTuplePtr& tp, bool, TestMap& self) -> OutTuplePtr {
		self.state()->cnt++;
		self.state()->sum += tp->getAttribute<2>();
		return makeTuplePtr(
			tp->getAttribute<0>(), tp->getAttribute<2>(),
			self.state()->cnt, self.state()->sum
		);
	};
	auto mop = std::make_shared< TestMap >(map_fun);

	CREATE_DATA_LINK(mockup, mop)
	CREATE_DATA_LINK(mop, mockup)

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}
