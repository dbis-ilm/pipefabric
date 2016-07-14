/*
 * AggrAvg.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
 */

#ifndef AGGRAVG_HPP_
#define AGGRAVG_HPP_

#include "AggregateFunc.hpp"

#include <type_traits>


namespace pfabric {

/**
 * @brief An aggregation function calculating a moving average.
 *
 * @tparam Tin
 *    the type of the input argument
 * @tparam Tres
 *    the type of the result (must be convertible from unsigned int)
 */
template<
	typename Tin,
	typename Tres
>
class AggrAvg :
	public AggregateFunc< Tin, Tres >
{
private:

	/// the type used for implementing the counter
	typedef unsigned int Count;

	static_assert( std::is_convertible< Count, Tres >::value,
		"result type must be convertible from a counter"
	);

public:

    AggrAvg() {
        init();
    }

    virtual void init() override {
    	mCount = 0;
        mSum = 0;
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        if (outdated) {
            mCount--;
            mSum -= data;
        } else {
            mCount++;
            mSum += data;
        }
    }

	virtual Tres value() override {
        return mSum / mCount;
    }

private:
    Count mCount;
    Tres mSum;
};


} /* end namespace pquery */


#endif /* AGGRAVG_HPP_ */
