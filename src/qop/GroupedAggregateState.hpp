/*
 * GroupedAggregateState.hpp
 *
 *  Created on: Feb 17, 2015
 *      Author: fbeier
 */

#ifndef GroupedAggregateState_hpp_
#define GroupedAggregateState_hpp_

#include "core/PFabricTypes.hpp"


namespace pfabric {


/**
 * @brief Base class for all grouped aggregation states.
 *
 * @tparam StreamElement
 *    the data stream element type used to build the state
 */
template<typename StreamElement>
class GroupedAggregateState {
public:
	GroupedAggregateState() : mCounter(1), mTstmp(0) {}
	virtual ~GroupedAggregateState() {}

	virtual void init() = 0;
	virtual GroupedAggregateState< StreamElement > *clone() const = 0;

	void updateCounter(int v) { mCounter += v; }
	int getCounter() const { return mCounter; }

  // do we really need timestamp here?
	Timestamp getTimestamp() const { return mTstmp; }
	void setTimestamp( Timestamp t) { mTstmp = t; }

	unsigned int mCounter;
	Timestamp mTstmp;
};

} /* end namespace pfabric */


#endif /* GroupedAggregateState_hpp_ */
