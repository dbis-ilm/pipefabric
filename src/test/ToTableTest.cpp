#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/ToTable.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int, std::string, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the projection operator.
 */
TEST_CASE("Writing a data stream to a table", "[ToTable]") {
  auto testTable = std::make_shared<Table<MyTuple, int>>("myTable");

	std::vector<MyTuplePtr> input = {
		makeTuplePtr(0, std::string("String #1"), 0),
		makeTuplePtr (1, std::string("String #2"), 10),
		makeTuplePtr(2, std::string("String #3"), 20) };

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, input);

	auto keyFunc = [&](const MyTuplePtr& tp) -> int { return tp->getAttribute<0>(); };
	auto op = std::make_shared< ToTable<MyTuplePtr, int> >(testTable, keyFunc);

	CREATE_DATA_LINK(mockup, op)

	mockup->start();

	REQUIRE(testTable->size() == 3);

  for (int i = 0; i < 3; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == fmt::format("String #{0}", i+1));
    REQUIRE(get<2>(tp) == i * 10);
  }
  testTable->drop();
}
