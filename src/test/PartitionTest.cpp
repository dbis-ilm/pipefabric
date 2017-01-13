#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "StreamMockup.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"
#include "qop/Merge.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/PartitionBy.hpp"
#include "qop/Where.hpp"

#include "fmt/format.h"

using namespace pfabric;

typedef Tuple<int, double, std::string> MyTuple;
typedef TuplePtr<MyTuple> MyTuplePtr;

/**
 * A test of the partition/merge operators.
 */
TEST_CASE("Partitioning a data stream and merging the results.",
          "[Partition][Merge]") {
  // we create some input data and the expected results for a filter "$0 % 2 ==
  // 0"
  const int numTuples = 1000;

  std::vector<MyTuplePtr> input, expected;

  for (int i = 0; i < numTuples; i++) {
    input.push_back(makeTuplePtr(i, i * 1.1, fmt::format("text{0}", i)));
    if (i % 2 == 0)
      expected.push_back(makeTuplePtr(i, i * 1.1, fmt::format("text{0}", i)));
  }

  // Due to the multi-threaded processing there is no guarantee that tuples
  // arrive in the same order as produced. Thus, StreamMockup has to sort the
  // the results using the given comparison function.
  auto mockup = std::make_shared<StreamMockup<MyTuplePtr, MyTuplePtr> >(
      input, expected, false, [](const MyTuplePtr& lhs, const MyTuplePtr& rhs) {
        return lhs->getAttribute<0>() < rhs->getAttribute<0>();
      });

  // create a PartitionBy instance with a partitioning function ($0 % 3)
  auto partition = std::make_shared<PartitionBy<MyTuplePtr> >(
      [&](const MyTuplePtr& tp) { return tp->getAttribute<0>() % 3; }, 3);
  CREATE_DATA_LINK(mockup, partition);

  // the filter predicate
  auto filter_fun = [&](const MyTuplePtr& tp, bool outdated) -> bool {
    // std::cout << "where: " << tp << std::endl;
    return tp->getAttribute<0>() % 2 == 0;
  };

  // for each partition we create a filter operator and register it for one
  // of the partition identifiers
  auto wop1 = std::make_shared<Where<MyTuplePtr> >(filter_fun);
  partition->connectChannelsForPartition(0, wop1->getInputDataChannel(),
                                         wop1->getInputPunctuationChannel());

  auto wop2 = std::make_shared<Where<MyTuplePtr> >(filter_fun);
  partition->connectChannelsForPartition(1, wop2->getInputDataChannel(),
                                         wop2->getInputPunctuationChannel());

  auto wop3 = std::make_shared<Where<MyTuplePtr> >(filter_fun);
  partition->connectChannelsForPartition(2, wop3->getInputDataChannel(),
                                         wop3->getInputPunctuationChannel());

  // finally, we create a merge operator to combine the results
  auto merge = std::make_shared<Merge<MyTuplePtr> >();
  CREATE_DATA_LINK(wop1, merge);
  CREATE_DATA_LINK(wop2, merge);
  CREATE_DATA_LINK(wop3, merge);

  CREATE_DATA_LINK(merge, mockup);

  mockup->start();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);

  REQUIRE(mockup->numTuplesProcessed() == numTuples / 2);
}
