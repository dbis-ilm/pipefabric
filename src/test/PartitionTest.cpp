#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Where.hpp"
#include "qop/PartitionBy.hpp"
#include "qop/Merge.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "StreamMockup.hpp"

#include "fmt/format.h"

using namespace pfabric;

typedef Tuple< int, double, std::string > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A test of the partition/merge operators.
 */
TEST_CASE("Applying a filter to a tuple stream", "[Partition][Merge]") {
	const int numTuples = 1000;

	std::vector<MyTuplePtr> input, expected;

	for (int i = 0; i < numTuples; i++) {
		input.push_back(makeTuplePtr(i, i * 1.1, fmt::format("text{0}", i)));
		if (i % 2 == 0)
			expected.push_back(makeTuplePtr(i, i * 1.1, fmt::format("text{0}", i)));
	}

	auto filter_fun = [&]( const MyTuplePtr& tp, bool outdated ) -> bool {
		return tp->getAttribute<0>() % 2 == 0;
	};

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);
	auto partition = std::make_shared<PartitionBy<MyTuplePtr> >(3);
	CREATE_DATA_LINK(mockup, partition);

	auto wop1 = std::make_shared< Where<MyTuplePtr> >(filter_fun);
	partition->setSubscriberForPartitionID(0, wop1);

	auto wop2 = std::make_shared< Where<MyTuplePtr> >(filter_fun);
	partition->setSubscriberForPartitionID(1, wop2);

	auto wop3 = std::make_shared< Where<MyTuplePtr> >(filter_fun);
	partition->setSubscriberForPartitionID(2, wop3);

	auto merge = std::make_shared<Merge<MyTuplePtr> >();
	CREATE_DATA_LINK(wop1, merge);
	CREATE_DATA_LINK(wop2, merge);
	CREATE_DATA_LINK(wop3, merge);

	CREATE_DATA_LINK(merge, mockup);

	mockup->start();
}
