/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef AggregateStateBase_hpp_
#define AggregateStateBase_hpp_

#include "core/PFabricTypes.hpp"
#include "core/StreamElementTraits.hpp"

namespace pfabric {


/**
 * @brief Base class for all aggregation states.
 *
 * AggregateStateBase is the base class for all aggregation state. An aggregation
 * state represents the intermediate state and results of an aggregation.
 *
 * @tparam StreamElement
 *    the data stream element type used to build the state
 */
template<typename StreamElement>
class AggregateStateBase {
public:
	/**
	 * Create a new AggregateStateBase instance.
	 */
	AggregateStateBase() : mCounter(1) /*, mTstmp(0) */ {}

	/**
	 * Deallocate all resources.
	 */
	virtual ~AggregateStateBase() {}

	/**
	 * Initialize (i.e. reset) the state of the aggregation.
	 */
	virtual void init() = 0;

	/**
	 * Add the given value to the internal counter.
	 *
	 * @param v the increment value
	 */
	void updateCounter(int v) { mCounter += v; }

	/**
	 * Return the current value of the counter.
	 *
	 * @return the counter value
	 */
	int getCounter() const { return mCounter; }


  /**
	 * Return the timestamp of the most recent update of the aggregate.
	 *
	 * @return a timestamp value
	 */
	Timestamp getTimestamp() const { return mTstmp; }

	/**
	 * Set the timestamp of the most recent update to the given value.
	 *
	 * @param a timestamp value
	 */
	void setTimestamp( Timestamp t) { mTstmp = t; }

	Timestamp mTstmp;      //< the timestamp of the most recent update
	unsigned int mCounter; //< counter for aggregation
};

/**
 * AggrStateTraits is a trait to check whether a class satisfies
 * the requirements of an aggregator class, i.e. to define a
 * @c ResultTypePtr.
 */
template<typename T>
struct AggrStateTraits : std::false_type{};

/**
 * Aggregator1 represents the aggregation state for a single aggregation
 * function.
 * The following example defines the aggregation state for calculate the sum
 * on column #0:
 ** @code
 * typedef Aggregator1<T1, AggrSum<double>, 0> MyAggrState;
 * @endcode
 *
 * @tparam StreamElement
 *         the data stream element type consumed by the aggregation
 * @tparam Aggr1Func
 *         the aggregation function
 * @tparam Aggr1Col
 *         the field number (0..n) in the input tuple used for aggregation
 */
template <typename StreamElement, typename Aggr1Func, int Aggr1Col>
class Aggregator1 : public AggregateStateBase<StreamElement> {
	Aggr1Func aggr1_;

public:
	/**
	 * The tuple type representing the aggregation result.
	 */
	typedef TuplePtr<typename Aggr1Func::ResultType> ResultTypePtr;

	/**
	 * Typedef for a pointer to the aggregation state.
	 */
	typedef std::shared_ptr<Aggregator1<StreamElement, Aggr1Func, Aggr1Col>> AggrStatePtr;

	/**
	 * Create a new aggregation state instance.
	 */
	Aggregator1() {}

	/**
	 * @see AggregateStateBase::init
	 */
	void init() override {
		aggr1_.init();
	}

	/**
	 * Process the input tuple @c tp and update the aggregation state
	 * object @c state according the aggregation function.
	 *
	 * @param tp the stream element processed in the aggregation
	 * @param state the aggregate state object
	 * @param outdated true if the tuple is outdated
	 */
	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
	}

	/**
	 * Return the current value of the given aggregate as a tuple.
	 * The method is a static member to simplify the usage in the aggregate operator.
	 *
	 * @param state the aggregate state object
	 * @return ResultTypePtr a pointer to a tuple representing the current aggregation state.
   */
	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value());
	}
};

template <typename StreamElement, typename Aggr1Func, int Aggr1Col>
struct AggrStateTraits<Aggregator1<StreamElement, Aggr1Func, Aggr1Col>> : std::true_type{};

/**
 * Aggregator2 represents the aggregation state for two aggregation
 * functions.
 *
 * @tparam StreamElement
 *         the data stream element type consumed by the aggregation
 * @tparam Aggr1Func
 *         the first aggregation function
 * @tparam Aggr1Col
 *         the field number (0..n) in the input tuple used for the first
 *         aggregation
 * @tparam Aggr2Func
 *         the second aggregation function
 * @tparam Aggr2Col
 *         the field number (0..n) in the input tuple used for the second
 *         aggregation
 */
template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col
>
class Aggregator2 : public AggregateStateBase<StreamElement> {
	Aggr1Func aggr1_;
	Aggr2Func aggr2_;

public:
	/**
	 * The tuple type representing the aggregation result.
	 */
	typedef TuplePtr<typename Aggr1Func::ResultType,
									 typename Aggr2Func::ResultType> ResultTypePtr;
	/**
	 * Typedef for a pointer to the aggregation state.
	 */
	typedef std::shared_ptr<Aggregator2<StreamElement, Aggr1Func, Aggr1Col,
		Aggr2Func, Aggr2Col>> AggrStatePtr;

		/**
		 * Create a new aggregation state instance.
		 */
	Aggregator2() {}

	/**
	 * @see AggregateStateBase::init
	 */
	void init() override {
		aggr1_.init();
		aggr2_.init();
	}

	/**
	 * Process the input tuple @c tp and update the aggregation state
	 * object @c state according the aggregation function.
	 *
	 * @param tp the stream element processed in the aggregation
	 * @param state the aggregate state object
	 * @param outdated true if the tuple is outdated
	 */
	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
		state->aggr2_.iterate(getAttribute<Aggr2Col>(*tp), outdated);
	}

	/**
	 * Return the current value of the given aggregate as a tuple.
	 * The method is a static member to simplify the usage in the aggregate operator.
	 *
	 * @param state the aggregate state object
	 * @return ResultTypePtr a pointer to a tuple representing the current aggregation state.
   */
	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value(), state->aggr2_.value());
	}
};

template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col
>
struct AggrStateTraits<Aggregator2<StreamElement, 
  Aggr1Func, Aggr1Col,
  Aggr2Func, Aggr2Col
  >> : std::true_type{};

/**
 * Aggregator3 represents the aggregation state for three aggregation
 * functions.
 *
 * @tparam StreamElement
 *         the data stream element type consumed by the aggregation
 * @tparam Aggr1Func
 *         the first aggregation function
 * @tparam Aggr1Col
 *         the field number (0..n) in the input tuple used for the first
 *         aggregation
 * @tparam Aggr2Func
 *         the second aggregation function
 * @tparam Aggr2Col
 *         the field number (0..n) in the input tuple used for the second
 *         aggregation
 * @tparam Aggr3Func
 *         the third aggregation function
 * @tparam Aggr3Col
 *         the field number (0..n) in the input tuple used for the third
 *         aggregation
 */
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
	/**
	 * The tuple type representing the aggregation result.
	 */
	typedef TuplePtr<typename Aggr1Func::ResultType,
									 typename Aggr2Func::ResultType,
									 typename Aggr3Func::ResultType> ResultTypePtr;

	/**
	 * Typedef for a pointer to the aggregation state.
	 */
	typedef std::shared_ptr<Aggregator3<StreamElement, Aggr1Func, Aggr1Col,
		Aggr2Func, Aggr2Col, Aggr3Func, Aggr3Col>> AggrStatePtr;

		/**
		 * Create a new aggregation state instance.
		 */
	Aggregator3() {}

	/**
	 * @see AggregateStateBase::init
	 */
	void init() override {
		aggr1_.init();
		aggr2_.init();
		aggr3_.init();
	}

	/**
	 * Process the input tuple @c tp and update the aggregation state
	 * object @c state according the aggregation function.
	 *
	 * @param tp the stream element processed in the aggregation
	 * @param state the aggregate state object
	 * @param outdated true if the tuple is outdated
	 */
	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
		state->aggr2_.iterate(getAttribute<Aggr2Col>(*tp), outdated);
		state->aggr3_.iterate(getAttribute<Aggr3Col>(*tp), outdated);
	}

	/**
	 * Return the current value of the given aggregate as a tuple.
	 * The method is a static member to simplify the usage in the aggregate operator.
	 *
	 * @param state the aggregate state object
	 * @return ResultTypePtr a pointer to a tuple representing the current aggregation state.
   */
	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value(), state->aggr2_.value(), state->aggr3_.value());
	}
};

template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col,
	typename Aggr3Func, int Aggr3Col
>
struct AggrStateTraits<Aggregator3<StreamElement, 
  Aggr1Func, Aggr1Col,
  Aggr2Func, Aggr2Col,
  Aggr3Func, Aggr3Col
  >> : std::true_type{};

/**
 * Aggregator4 represents the aggregation state for four aggregation
 * functions.
 *
 * @tparam StreamElement
 *         the data stream element type consumed by the aggregation
 * @tparam Aggr1Func
 *         the first aggregation function
 * @tparam Aggr1Col
 *         the field number (0..n) in the input tuple used for the first
 *         aggregation
 * @tparam Aggr2Func
 *         the second aggregation function
 * @tparam Aggr2Col
 *         the field number (0..n) in the input tuple used for the second
 *         aggregation
 * @tparam Aggr3Func
 *         the third aggregation function
 * @tparam Aggr3Col
 *         the field number (0..n) in the input tuple used for the third
 *         aggregation
 * @tparam Aggr4Func
 *         the fourth aggregation function
 * @tparam Aggr4Col
 *         the field number (0..n) in the input tuple used for the fourth
 *         aggregation
 */
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
	/**
	 * The tuple type representing the aggregation result.
	 */
	typedef TuplePtr<typename Aggr1Func::ResultType,
									 typename Aggr2Func::ResultType,
									 typename Aggr3Func::ResultType,
									 typename Aggr4Func::ResultType> ResultTypePtr;

	/**
	 * Typedef for a pointer to the aggregation state.
	 */
	typedef std::shared_ptr<Aggregator4<StreamElement, Aggr1Func, Aggr1Col,
		Aggr2Func, Aggr2Col, Aggr3Func, Aggr3Col, Aggr4Func, Aggr4Col>> AggrStatePtr;

		/**
		 * Create a new aggregation state instance.
		 */
	Aggregator4() {}

	/**
	 * @see AggregateStateBase::init
	 */
	void init() override {
		aggr1_.init();
		aggr2_.init();
		aggr3_.init();
		aggr4_.init();
	}

	/**
	 * Process the input tuple @c tp and update the aggregation state
	 * object @c state according the aggregation function.
	 *
	 * @param tp the stream element processed in the aggregation
	 * @param state the aggregate state object
	 * @param outdated true if the tuple is outdated
	 */
	static void iterate(const StreamElement& tp, AggrStatePtr state, const bool outdated) {
		state->aggr1_.iterate(getAttribute<Aggr1Col>(*tp), outdated);
		state->aggr2_.iterate(getAttribute<Aggr2Col>(*tp), outdated);
		state->aggr3_.iterate(getAttribute<Aggr3Col>(*tp), outdated);
		state->aggr4_.iterate(getAttribute<Aggr4Col>(*tp), outdated);
	}

	/**
	 * Return the current value of the given aggregate as a tuple.
	 * The method is a static member to simplify the usage in the aggregate operator.
	 *
	 * @param state the aggregate state object
	 * @return ResultTypePtr a pointer to a tuple representing the current aggregation state.
   */
	static ResultTypePtr finalize(AggrStatePtr state) {
		return makeTuplePtr(state->aggr1_.value(), state->aggr2_.value(),
												state->aggr3_.value(), state->aggr4_.value());
	}
};

template <
	typename StreamElement,
	typename Aggr1Func, int Aggr1Col,
	typename Aggr2Func, int Aggr2Col,
	typename Aggr3Func, int Aggr3Col,
	typename Aggr4Func, int Aggr4Col
>
struct AggrStateTraits<Aggregator4<StreamElement, 
  Aggr1Func, Aggr1Col,
  Aggr2Func, Aggr2Col,
  Aggr3Func, Aggr3Col,
  Aggr4Func, Aggr4Col
  >> : std::true_type{};

} /* end namespace pfabric */


#endif /* AggregateStateBase_hpp_ */
