#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "core/Tuple.hpp"
#include "qop/AggregateFunctions.hpp"
#include "qop/Aggregation.hpp"
#include "qop/SlidingWindow.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple<double> InTuple;
typedef TuplePtr<InTuple> InTuplePtr;
typedef Tuple<double, double, int> OutTuple;
typedef TuplePtr<OutTuple> OutTuplePtr;

typedef Tuple<double, double, double, double> Out2Tuple;
typedef TuplePtr<Out2Tuple> Out2TuplePtr;

class MyAggregateState : public BaseAggregateState {
public:
	AggrSum<double> sum1_;
	AggrAvg<double, double> avg2_;
	AggrCount<double, int> cnt3_;

	MyAggregateState() {}

	virtual void init() override {
		sum1_.init();
		avg2_.init();
		cnt3_.init();
	}

	// TODO no value clones?
	virtual AggregateStatePtr clone() const override {
		return std::make_shared<MyAggregateState>();
	}
};

TEST_CASE( "Compute a simple aggregate on the entire stream", "[Aggregation]" ) {
	typedef Aggregation<InTuplePtr, OutTuplePtr> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(1.0), makeTuplePtr(2.0), makeTuplePtr(3.0),
		makeTuplePtr(4.0), makeTuplePtr(5.0), makeTuplePtr(6.0) };

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr (21.0, 3.5, 6)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);

	auto finalFun = [&](AggregateStatePtr state) {
		auto myState = dynamic_cast<MyAggregateState *>(state.get());
		return makeTuplePtr(myState->sum1_.value(), myState->avg2_.value(), myState->cnt3_.value());
	};

	auto iterFun = [&]( const InTuplePtr& tp, AggregateStatePtr state, const bool outdated) {
		auto myState = dynamic_cast<MyAggregateState *>(state.get());
		myState->sum1_.iterate(tp->getAttribute<0>(), outdated);
		myState->avg2_.iterate(tp->getAttribute<0>(), outdated);
		myState->cnt3_.iterate(tp->getAttribute<0>(), outdated);
	};

	auto aggr = std::make_shared<TestAggregation>(
							   std::make_shared<MyAggregateState>(), finalFun, iterFun, TriggerByCount, 100);

	CREATE_LINK(mockup, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();
}

TEST_CASE( "Compute an incremental aggregate on the entire stream", "[Aggregation]" ) {
	typedef Aggregation<InTuplePtr, OutTuplePtr> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(1.0), makeTuplePtr(2.0), makeTuplePtr(3.0),
		makeTuplePtr(4.0), makeTuplePtr(5.0), makeTuplePtr(6.0)
	};

	std::vector<OutTuplePtr> expected = {
		makeTuplePtr(1.0, 1.0, 1),
		makeTuplePtr(3.0, 1.5, 2),
		makeTuplePtr(6.0, 2.0, 3),
		makeTuplePtr(10.0, 2.5, 4),
		makeTuplePtr(15.0, 3.0, 5),
		makeTuplePtr(21.0, 3.5, 6)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);

	auto finalFun = [&](AggregateStatePtr state) {
		auto myState = dynamic_cast<MyAggregateState *>(state.get());
		return makeTuplePtr(myState->sum1_.value(), myState->avg2_.value(), myState->cnt3_.value());
	};

	auto iterFun = [&]( const InTuplePtr& tp, AggregateStatePtr state, const bool outdated) {
		auto myState = dynamic_cast<MyAggregateState *>(state.get());
		myState->sum1_.iterate(tp->getAttribute<0>(), outdated);
		myState->avg2_.iterate(tp->getAttribute<0>(), outdated);
		myState->cnt3_.iterate(tp->getAttribute<0>(), outdated);
	};

	auto aggr = std::make_shared<TestAggregation>(
		std::make_shared<MyAggregateState>(), finalFun, iterFun
	);

	CREATE_LINK(mockup, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();
}

class MyAggregate2State :	public BaseAggregateState {
public:
	AggrMinMax<double, std::less<double>> min1_;
	AggrMinMax<double, std::greater<double>> max2_;
	AggrMRecent<double> mrecent3_;
	AggrLRecent<double> lrecent4_;
	MyAggregate2State() {}

	virtual void init() override {
		min1_.init();
		max2_.init();
		mrecent3_.init();
		lrecent4_.init();
	}

	virtual AggregateStatePtr clone() const override {
		return std::make_shared<MyAggregate2State>();
	}
};


TEST_CASE( "Compute an incremental min/maxaggregate on the stream", "[Aggregation]" ) {
	typedef Aggregation<InTuplePtr, Out2TuplePtr> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(3.4), makeTuplePtr(2.1), makeTuplePtr(3.0),
		makeTuplePtr(5.7), makeTuplePtr(9.1), makeTuplePtr(7.4)
	};

	std::vector<Out2TuplePtr> expected = {
		makeTuplePtr(3.4, 3.4, 3.4, 3.4), makeTuplePtr(2.1, 3.4, 2.1, 3.4),
		makeTuplePtr(2.1, 3.4, 3.0, 3.4), makeTuplePtr(2.1, 5.7, 5.7, 3.4),
		makeTuplePtr(2.1, 9.1, 9.1, 3.4), makeTuplePtr(2.1, 9.1, 7.4, 3.4)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, Out2TuplePtr> >(input, expected);

		auto finalFun = [&](AggregateStatePtr state) {
		auto myState = dynamic_cast<MyAggregate2State *>(state.get());
		return makeTuplePtr(myState->min1_.value(), myState->max2_.value(), myState->mrecent3_.value(), myState->lrecent4_.value());
	};

	auto iterFun = [&](const InTuplePtr& tp, AggregateStatePtr state, const bool outdated) {
		auto myState = dynamic_cast<MyAggregate2State *>(state.get());
		myState->min1_.iterate(tp->getAttribute<0>(), outdated);
		myState->max2_.iterate(tp->getAttribute<0>(), outdated);
		myState->mrecent3_.iterate(tp->getAttribute<0>(), outdated);
		myState->lrecent4_.iterate(tp->getAttribute<0>(), outdated);
	};

	auto aggr = std::make_shared<TestAggregation>(
		std::make_shared<MyAggregate2State>(), finalFun, iterFun
	);

	CREATE_LINK(mockup, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();
}

TEST_CASE( "Compute an incremental min/maxaggregate on a window", "[Aggregation]" ) {
	typedef Aggregation<InTuplePtr, Out2TuplePtr> TestAggregation;

	std::vector<InTuplePtr> input = {
		makeTuplePtr(3.4), makeTuplePtr(2.1), makeTuplePtr(3.0),
		makeTuplePtr(5.7), makeTuplePtr(9.1), makeTuplePtr(7.4)
	};

	// auto resultFile = this->getTestFile( "aggr_test_mm_win.res" );
	std::vector<Out2TuplePtr> expected = {
		makeTuplePtr(3.4, 3.4, 3.4, 3.4),
		makeTuplePtr(2.1, 3.4, 2.1, 3.4),
		makeTuplePtr(2.1, 3.4, 3.0, 3.4),
		makeTuplePtr(2.1, 3.0, 3.0, 2.1), // outdated
		makeTuplePtr(2.1, 5.7, 5.7, 2.1),
		makeTuplePtr(3.0, 5.7, 5.7, 3.0), // outdated
		makeTuplePtr(3.0, 9.1, 9.1, 3.0),
		makeTuplePtr(5.7, 9.1, 9.1, 5.7), // outdated
		makeTuplePtr(5.7, 9.1, 7.4, 5.7)
	};

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, Out2TuplePtr> >(input, expected);
	auto win = std::make_shared<SlidingWindow<InTuplePtr>>(WindowParams::RowWindow, 3);

	auto finalFun = [&](AggregateStatePtr state) {
		auto myState = dynamic_cast<MyAggregate2State *>(state.get());
		return makeTuplePtr(myState->min1_.value(), myState->max2_.value(), myState->mrecent3_.value(), myState->lrecent4_.value());
	};

	auto iterFun = [&](const InTuplePtr& tp, AggregateStatePtr state, const bool outdated) {
		auto myState = dynamic_cast<MyAggregate2State *>(state.get());
		myState->min1_.iterate(tp->getAttribute<0>(), outdated);
		myState->max2_.iterate(tp->getAttribute<0>(), outdated);
		myState->mrecent3_.iterate(tp->getAttribute<0>(), outdated);
		myState->lrecent4_.iterate(tp->getAttribute<0>(), outdated);
	};

	auto aggr = std::make_shared<TestAggregation>(
		std::make_shared<MyAggregate2State>(), finalFun, iterFun
	);

	CREATE_LINK(mockup, win);
	CREATE_LINK(win, aggr);
	CREATE_LINK(aggr, mockup);

	mockup->start();
}
