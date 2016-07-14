/*
 * AggrGlobalMin.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
 */

#ifndef AGGRGLOBALMIN_HPP_
#define AGGRGLOBALMIN_HPP_

#include "AggregateFunc.hpp"

#include <cassert>
#include <string>
#include <limits>


namespace pfabric {

/**
 * @brief An aggregation determining the minimum value in a stream.
 *
 * @tparam Tin
 *    the type of the input argument
 */
template<
	typename Tin
>
class AggrGlobalMin : public AggregateFunc<Tin, Tin> {
public:

    AggrGlobalMin() {
        init();
    }

    virtual void init() override {
        mMin = std::numeric_limits<Tin>::max();
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        assert(!outdated);
        mMin = std::min(mMin, data);
    }

    virtual Tin value() override {
        return mMin;
    }

private:
    Tin mMin;
};

/*
 * TODO Question: What is this?
 */
template<>
inline void AggrGlobalMin<std::string>::init() {
	mMin = 0x7f;
}


} /* end namespace pquery */


#endif /* AGGRGLOBALMIN_HPP_ */
