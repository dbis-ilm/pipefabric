/*
 * AggregateStateBase.hpp
 *
 *  Created on: Feb 17, 2015
 *      Author: fbeier
 */

#ifndef AggregateStateBase_hpp_
#define AggregateStateBase_hpp_

#include "core/PFabricTypes.hpp"
#include "core/StreamElementTraits.hpp"

namespace pfabric {


/**
 * @brief Base class for all aggregation states.
 *
 * @tparam StreamElement
 *    the data stream element type used to build the state
 */
template<typename StreamElement>
class AggregateStateBase {
public:
	AggregateStateBase() : mCounter(1), mTstmp(0) {}
	virtual ~AggregateStateBase() {}

	virtual void init() = 0;
	virtual AggregateStateBase< StreamElement > *clone() const = 0;

	void updateCounter(int v) { mCounter += v; }
	int getCounter() const { return mCounter; }

  // do we really need timestamp here?
	Timestamp getTimestamp() const { return mTstmp; }
	void setTimestamp( Timestamp t) { mTstmp = t; }

	unsigned int mCounter;
	Timestamp mTstmp;
};

template <typename StreamElement, typename Aggr1Func, int Aggr1Col>
class Aggregator1 : public AggregateStateBase<StreamElement> {
	Aggr1Func aggr1_;

public:
	typedef TuplePtr<Tuple<typename Aggr1Func::ResultType>> ResultTypePtr;
	typedef std::shared_ptr<Aggregator1<StreamElement, Aggr1Func, Aggr1Col>> AggrStatePtr;

	Aggregator1() {}

	void init() override {
		aggr1_.init();
	}

	AggregateStateBase<StreamElement> *clone() const override {
		return new Aggregator1< StreamElement, Aggr1Func, Aggr1Col >();
	}

	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
	}

	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value());
	}
};

template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col
>
class Aggregator2 : public AggregateStateBase<StreamElement> {
	Aggr1Func aggr1_;
	Aggr2Func aggr2_;

public:
	typedef TuplePtr<Tuple<typename Aggr1Func::ResultType,
												 typename Aggr2Func::ResultType>> ResultTypePtr;
	typedef std::shared_ptr<Aggregator2<StreamElement, Aggr1Func, Aggr1Col,
		Aggr2Func, Aggr2Col>> AggrStatePtr;

	Aggregator2() {}

	void init() override {
		aggr1_.init();
		aggr2_.init();
	}

	AggregateStateBase<StreamElement> *clone() const override {
		return new Aggregator2< StreamElement, Aggr1Func, Aggr1Col,
			Aggr2Func, Aggr2Col >();
	}

	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
		state->aggr2_.iterate(getAttribute<Aggr2Col>(*tp), outdated);
	}

	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value(), state->aggr2_.value());
	}
};

template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col,
	typename Aggr3Func, int Aggr3Col
>
class Aggregator3 : public AggregateStateBase<StreamElement> {
	Aggr1Func aggr1_;
	Aggr2Func aggr2_;
	Aggr3Func aggr3_;

public:
	typedef TuplePtr<Tuple<typename Aggr1Func::ResultType,
												 typename Aggr2Func::ResultType,
												 typename Aggr3Func::ResultType>> ResultTypePtr;
	typedef std::shared_ptr<Aggregator3<StreamElement, Aggr1Func, Aggr1Col,
		Aggr2Func, Aggr2Col, Aggr3Func, Aggr3Col>> AggrStatePtr;

	Aggregator3() {}

	void init() override {
		aggr1_.init();
		aggr2_.init();
		aggr3_.init();
	}

	AggregateStateBase<StreamElement> *clone() const override {
		return new Aggregator3< StreamElement, Aggr1Func, Aggr1Col,
			Aggr2Func, Aggr2Col, Aggr3Func, Aggr3Col >();
	}

	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
		state->aggr2_.iterate(getAttribute<Aggr2Col>(*tp), outdated);
		state->aggr3_.iterate(getAttribute<Aggr3Col>(*tp), outdated);
	}

	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value(), state->aggr2_.value(), state->aggr3_.value());
	}
};

template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col,
	typename Aggr3Func, int Aggr3Col,
	typename Aggr4Func, int Aggr4Col
>
class Aggregator4 : public AggregateStateBase<StreamElement> {
	Aggr1Func aggr1_;
	Aggr2Func aggr2_;
	Aggr3Func aggr3_;
	Aggr4Func aggr4_;

public:
	typedef TuplePtr<Tuple<typename Aggr1Func::ResultType,
												 typename Aggr2Func::ResultType,
												 typename Aggr3Func::ResultType,
												 typename Aggr4Func::ResultType>> ResultTypePtr;
	typedef std::shared_ptr<Aggregator4<StreamElement, Aggr1Func, Aggr1Col,
		Aggr2Func, Aggr2Col, Aggr3Func, Aggr3Col, Aggr4Func, Aggr4Col>> AggrStatePtr;

	Aggregator4() {}

	void init() override {
		aggr1_.init();
		aggr2_.init();
		aggr3_.init();
		aggr4_.init();
	}

	AggregateStateBase<StreamElement> *clone() const override {
		return new Aggregator4< StreamElement, Aggr1Func, Aggr1Col,
			Aggr2Func, Aggr2Col, Aggr3Func, Aggr3Col, Aggr4Func, Aggr4Col >();
	}

	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
		state->aggr2_.iterate(getAttribute<Aggr2Col>(*tp), outdated);
		state->aggr3_.iterate(getAttribute<Aggr3Col>(*tp), outdated);
		state->aggr4_.iterate(getAttribute<Aggr4Col>(*tp), outdated);
	}

	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value(), state->aggr2_.value(),
												state->aggr3_.value(), state->aggr4_.value());
	}
};
} /* end namespace pfabric */


#endif /* AggregateStateBase_hpp_ */
