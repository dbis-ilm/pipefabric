#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"


#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "core/Tuple.hpp"
#include "core/Punctuation.hpp"
#include "qop/GroupedAggregation.hpp"
#include "qop/AggregateFunctions.hpp"
#include "qop/SlidingWindow.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;


typedef TuplePtr<int, double> InTuplePtr;
typedef TuplePtr<int, double, double, int> OutTuplePtr;
typedef TuplePtr<int, double, double, double> OutTuple2Ptr;

/*----------------------------------------------------------------------- */

template< typename StreamElement >
class MyAggregateState : public AggregateStateBase< StreamElement > {
public:
	int group1_;
	AggrSum<double> sum1_;
	AggrAvg<double, double> avg2_;
	AggrCount<double, int> cnt3_;

	MyAggregateState() { init(); }

	virtual void init() override {
		group1_ = 0;
		sum1_.init();
		avg2_.init();
		cnt3_.init();
	}
};


/* ------------------------------------------------------------------------------------------------------------------------------ */

/**
* A simple test of the grouped aggregation operator.
*
* TODO random?
* We generate tuples with [random-value] and
* calculate [timestamp, sum(random-value), avg(random-value), count(*)].
* After receiving a punctuation tuple the result is checked.
*/
TEST_CASE( "Compute a simple punctuation based groupby with aggregates", "[GroupedAggregation]" ) {
	typedef MyAggregateState< const InTuplePtr& > MyAggrState;
	typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	typedef GroupedAggregation<const InTuplePtr&, const OutTuplePtr&, MyAggrState > TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(1,3.4), makeTuplePtr(2,9.1), makeTuplePtr(3,5.7),
		makeTuplePtr(3,2.1), makeTuplePtr(1,2.1), makeTuplePtr(3,3.0),
		makeTuplePtr(1,3.0), makeTuplePtr(2,2.1), makeTuplePtr(1,5.7),
		makeTuplePtr(1,9.1), makeTuplePtr(2,7.4), makeTuplePtr(3,3.4),
		makeTuplePtr(2,3.0), makeTuplePtr(3,7.4), makeTuplePtr(2,3.4),
		makeTuplePtr(2,5.7), makeTuplePtr(1,7.4), makeTuplePtr(3,9.1),
		};

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr(1,30.7,5.116667,6),
		makeTuplePtr(2,30.7,5.116667,6),
		makeTuplePtr(3,30.7,5.116667,6)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);

	auto aggr = std::make_shared< TestAggregation >(
		/* key function */
		[&](const InTuplePtr& tp) { return tp->getAttribute<0>(); },
		/* final function */
		[&](MyAggrStatePtr myState) {
			auto tp = makeTuplePtr(myState->group1_, myState->sum1_.value(),
				myState->avg2_.value(), myState->cnt3_.value());
			return tp;
		},
		/* iterate function */
		[&](const InTuplePtr& tp, MyAggrStatePtr myState, const bool outdated) {
			myState->group1_ = tp->getAttribute<0>();
			myState->sum1_.iterate(tp->getAttribute<1>(), outdated);
			myState->avg2_.iterate(tp->getAttribute<1>(), outdated);
			myState->cnt3_.iterate(tp->getAttribute<1>(), outdated);
		},
		TriggerByCount, 10000
	);

	CREATE_LINK( mockup, aggr );
	CREATE_DATA_LINK( aggr, mockup );

	mockup->start();
}


/*-------------------------------------------------------------------------- */

template< typename StreamElement >
class MyAggregateState2 : public AggregateStateBase< StreamElement > {
public:
	int group1_;
	AggrMinMax<double, std::less<double>> min1_;
	AggrMinMax<double, std::greater<double>> max2_;
	AggrLRecent<double> lrecent3_;

	MyAggregateState2() { init(); }

	virtual void init() override {
		group1_ = 0;
		min1_.init();
		max2_.init();
		lrecent3_.init();
	}

};


TEST_CASE( "Compute a groupby with incremental min/max aggregates", "[GroupedAggregation]" ) {
	typedef MyAggregateState2< const InTuplePtr& > MyAggrState2;
	typedef std::shared_ptr<MyAggrState2> MyAggrState2Ptr;
	typedef GroupedAggregation<InTuplePtr, OutTuple2Ptr, MyAggrState2> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(1,3.4), makeTuplePtr(2,9.1), makeTuplePtr(3,5.7),
		makeTuplePtr(3,2.1), makeTuplePtr(1,2.1), makeTuplePtr(3,3.0),
		makeTuplePtr(1,3.0), makeTuplePtr(2,2.1), makeTuplePtr(1,5.7),
		makeTuplePtr(1,9.1), makeTuplePtr(2,7.4), makeTuplePtr(3,3.4),
		makeTuplePtr(2,3.0), makeTuplePtr(3,7.4), makeTuplePtr(2,3.4),
		makeTuplePtr(2,5.7), makeTuplePtr(1,7.4), makeTuplePtr(3,9.1),
		};

	std::vector<OutTuple2Ptr> expected = {
		makeTuplePtr(1,3.4,3.4,3.4), makeTuplePtr(2,9.1,9.1,9.1), makeTuplePtr(3,5.7,5.7,5.7),
		makeTuplePtr(3,2.1,5.7,5.7), makeTuplePtr(1,2.1,3.4,3.4), makeTuplePtr(3,2.1,5.7,5.7),
		makeTuplePtr(1,2.1,3.4,3.4), makeTuplePtr(2,2.1,9.1,9.1), makeTuplePtr(1,2.1,5.7,3.4),
		makeTuplePtr(1,2.1,9.1,3.4), makeTuplePtr(2,2.1,9.1,9.1), makeTuplePtr(3,2.1,5.7,5.7),
		makeTuplePtr(2,2.1,9.1,9.1), makeTuplePtr(3,2.1,7.4,5.7), makeTuplePtr(2,2.1,9.1,9.1),
		makeTuplePtr(2,2.1,9.1,9.1), makeTuplePtr(1,2.1,9.1,3.4), makeTuplePtr(3,2.1,9.1,5.7)
	};


	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuple2Ptr> >(input, expected);

	auto aggr = std::make_shared< TestAggregation >(
		/* key function */
		[&](const InTuplePtr& tp) { return tp->getAttribute<0>(); },
		/* final function */
		[&](MyAggrState2Ptr myState) {
			auto tp = makeTuplePtr(myState->group1_, myState->min1_.value(),
				myState->max2_.value(), myState->lrecent3_.value());
			return tp;
		},
		/* iterate function */
		[&](const InTuplePtr& tp, MyAggrState2Ptr myState, const bool outdated) {
			myState->group1_ = tp->getAttribute<0>();
			myState->min1_.iterate(tp->getAttribute<1>(), outdated);
			myState->max2_.iterate(tp->getAttribute<1>(), outdated);
			myState->lrecent3_.iterate(tp->getAttribute<1>(), outdated);
		});

	CREATE_LINK( mockup, aggr );
	CREATE_DATA_LINK( aggr, mockup );

	mockup->start();
}
