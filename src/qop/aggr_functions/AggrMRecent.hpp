/*
 * AggrMRecent.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
 */

#ifndef AGGRMRECENT_HPP_
#define AGGRMRECENT_HPP_

#include "AggregateFunc.hpp"
#include "core/PFabricTypes.hpp"
#include <type_traits>


namespace pfabric {

/**
 * @brief An aggregation determining the oldest valid value in a stream.
 *
 * @tparam Tin
 *    the type of the input argument
 */
template<
	typename Tin
>
class AggrMRecent :
	public AggregateFunc< Tin, Tin >
{
private:

	static_assert( std::is_copy_assignable< Tin >::value,
		"input type must be copy assignable"
	);

public:

    AggrMRecent() {
        init();
    }

    virtual void init() override {
    	mMostRecentTime = 0;
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        if (!outdated) {
            mVal = data;
        }
    }

    /*
	 * TODO Question: Is the iteration with timestamp required in general?
	 *                Then we should include this in the base class to keep the interface consistent.
	 */
	void iterate(Tin const& data, const Timestamp& ts, bool outdated = false) {
		if (!outdated && ts > mMostRecentTime) {
			mVal = data;
			mMostRecentTime = ts;
		}
	}

	virtual Tin value() override {
        return mVal;
    }

private:
    Tin mVal;
    Timestamp mMostRecentTime;
};

} /* end namespace pquery */


#endif /* AGGRMRECENT_HPP_ */
