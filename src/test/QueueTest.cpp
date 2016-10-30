#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Queue.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int, int, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the projection operator.
 */
TEST_CASE("Decoupling producer and consumer via a queue", "[Queue]") {
  std::vector<MyTuplePtr> input = {
    makeTuplePtr(0, 0, 0),
    makeTuplePtr (1, 1, 10),
    makeTuplePtr(2, 2, 20) };
  
  std::vector<MyTuplePtr> expected = {
    makeTuplePtr(0, 0, 0),
    makeTuplePtr (1, 1, 10),
    makeTuplePtr(2, 2, 20) };
  
  auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);
  
  auto ch = std::make_shared<Queue<MyTuplePtr> >();

  CREATE_DATA_LINK(mockup, ch)
  CREATE_DATA_LINK(ch, mockup)
    
  mockup->start();
  
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}
