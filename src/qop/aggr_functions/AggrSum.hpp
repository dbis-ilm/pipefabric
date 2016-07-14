/*
 * AggrSum.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
 */

#ifndef AGGRSUM_HPP_
#define AGGRSUM_HPP_

#include "AggregateFunc.hpp"


namespace pfabric {

/**
 * @brief A summation aggregation.
 *
 * @tparam Tin
 *    the type of the input argument
 */
template<
	typename Tin
>
class AggrSum :
	public AggregateFunc< Tin, Tin >
{
public:

    AggrSum() {
        init();
    }

    virtual void init() override {
        mSum = 0;
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        mSum += (outdated ? -data : data);
    }

    virtual Tin value() override {
        return mSum;
    }

private:
    Tin mSum;
};

} /* end namespace pquery */



#endif /* AGGRSUM_HPP_ */
