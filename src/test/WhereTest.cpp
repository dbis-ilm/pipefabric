#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

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
