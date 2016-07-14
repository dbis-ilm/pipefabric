/*
 * AggrGlobalMax.hpp
 *
 *  Created on: Apr 2, 2015
 *      Author: fbeier
 */

#ifndef AGGRGLOBALMAX_HPP_
#define AGGRGLOBALMAX_HPP_

#include "AggregateFunc.hpp"

#include <cassert>
#include <string>
#include <limits>


namespace pfabric {

/**
 * @brief An aggregation determining the maximum value in a stream.
 *
 * @tparam Tin
 *    the type of the input argument
 */
template<
	typename Tin
>
class AggrGlobalMax : public AggregateFunc<Tin, Tin> {
public:

	AggrGlobalMax() {
        init();
    }

    virtual void init() override {
    	mMax = std::numeric_limits<Tin>::min();
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        assert(!outdated);
        mMax = std::max(mMax, data);
    }

    virtual Tin value() override {
        return mMax;
    }

private:
    Tin mMax;
};

/*
 * TODO Question: What is this?
 */
template<>
inline void AggrGlobalMax<std::string>::init() {
	mMax = 0x00f;
}


} /* end namespace pquery */


#endif /* AGGRGLOBALMAX_HPP_ */
