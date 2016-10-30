#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Map.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int, int, int > InTuple;
typedef TuplePtr< InTuple > InTuplePtr;

typedef Tuple< int, int, int, int > OutTuple;
typedef TuplePtr< OutTuple > OutTuplePtr;

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

	auto map_fun = [&]( const InTuplePtr& tp ) -> OutTuplePtr {
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
