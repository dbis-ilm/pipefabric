#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Notify.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int, int, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the notify operator.
 */
TEST_CASE("Invoking callbacks on a tuple stream", "[Notify]") {
	std::vector<MyTuplePtr> input = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr (1, 1, 10),
		makeTuplePtr(2, 2, 20) };

	std::vector<MyTuplePtr> expected = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr (1, 1, 10),
		makeTuplePtr(2, 2, 20) };

	std::vector<MyTuplePtr> callbackTuples;

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);

	auto nop = std::make_shared< Notify<MyTuplePtr> >([&](auto tp, bool outdated) {
		callbackTuples.push_back(tp);
	});

	CREATE_DATA_LINK(mockup, nop)
	CREATE_DATA_LINK(nop, mockup)

	mockup->start();

	auto lambda = [] (auto a, auto b) { return a->data() == b->data(); };

	REQUIRE(std::equal(callbackTuples.begin(),
										callbackTuples.begin() + 3,
										expected.begin(),
										lambda));
}
