#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>
#include <chrono>
#include <future>

#include "core/Tuple.hpp"
#include "qop/Queue.hpp"
#include "qop/Barrier.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

struct BarrierCounter {
		std::atomic<int> counter;
		std::condition_variable cVar;
		std::mutex mtx;

		BarrierCounter() { counter.store(0); }

		void set(int v) {
			std::unique_lock<std::mutex> lock(mtx);
			counter.store(v);
			cVar.notify_one();
		}

		int get() const { return counter.load(); }
};

/**
 * A simple test of the barrier operator.
 */
TEST_CASE("Controlling stream processing by a barrier", "[Barrier]") {
	using namespace std::chrono_literals;

	std::vector<MyTuplePtr> input = {
		makeTuplePtr(1),
		makeTuplePtr(2),
		makeTuplePtr(3),
		makeTuplePtr(4),
		makeTuplePtr(11),
		makeTuplePtr(12),
		makeTuplePtr(20),
		makeTuplePtr(21),
		makeTuplePtr(22)
	};
	std::vector<MyTuplePtr> expected = {
		makeTuplePtr(1),
		makeTuplePtr(2),
		makeTuplePtr(3),
		makeTuplePtr(4)
	};

	BarrierCounter counter;

	counter.set(10);

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(input, expected);

	auto ch = std::make_shared<Queue<MyTuplePtr> >();

	auto barrier = std::make_shared< Barrier<MyTuplePtr> >(counter.cVar, counter.mtx, [&](auto tp) -> bool {
		return getAttribute<0>(tp) < counter.get();
	});

	CREATE_DATA_LINK(mockup, ch)
	CREATE_DATA_LINK(ch, barrier)
	CREATE_DATA_LINK(barrier, mockup)

	// set counter to 10 and send tuples 1, 2, 3, 4, 11, 12:
	// => only tuples 1, 2, 3, 4 should arrive
	mockup->start();

	std::this_thread::sleep_for(1s);
	REQUIRE(mockup->numTuplesProcessed() == 4);

	// now set counter to 13:
	// => we expect 11, 12 as results
	mockup->addExpected({ makeTuplePtr(11), makeTuplePtr(12) });
	counter.set(13);

	std::this_thread::sleep_for(1s);
	REQUIRE(mockup->numTuplesProcessed() == 6);

	// set counter to 25:
	// => we receive 20, 21, 22
	mockup->addExpected({ makeTuplePtr(20), makeTuplePtr(21), makeTuplePtr(22) });
	counter.set(25);

	std::this_thread::sleep_for(1s);
	REQUIRE(mockup->numTuplesProcessed() == 9);
}
