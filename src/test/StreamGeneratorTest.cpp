#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/StreamGenerator.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int, int, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the StreamGenerator operator.
 */
TEST_CASE("Producing a data stream using the StreamGenerator operator", "[StreamGenerator]") {

	std::vector<MyTuplePtr> expected;
  for (int i = 0; i < 1000; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    expected.push_back(tp);
  }

  auto op = std::make_shared<StreamGenerator<MyTuplePtr> >([](unsigned long n) -> MyTuplePtr {
      return makeTuplePtr((int)n, (int)n + 10, (int)n + 100);
      }, 1000);
	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(expected, expected);
	CREATE_DATA_LINK(op, mockup);

  op->start();
  REQUIRE(mockup->numTuplesProcessed() == 1000);
}

