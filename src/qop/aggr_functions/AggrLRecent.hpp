/*
 * AggrLRecent.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
 */

#ifndef AGGRLRECENT_HPP_
#define AGGRLRECENT_HPP_

#include "AggregateFunc.hpp"

#include <list>
#include <cassert>


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
class AggrLRecent :
	public AggregateFunc< Tin, Tin >
{
private:

	/// a list with all values processed so far
	typedef std::list<Tin> ValueList;

public:

    AggrLRecent() {
        init();
    }

    virtual void init() override {
    	mData.clear();
    }

    virtual void iterate(Tin const& data, bool outdated = false) override {
        if (!outdated) {
            mData.push_back(data);
        } else {
        	assert( !mData.empty() );
            mData.pop_front();
        }
    }

    virtual Tin value() override {
    	assert( !mData.empty() );
        return mData.front();
    }

private:
    ValueList mData;
};

} /* end namespace pquery */


#endif /* AGGRLRECENT_HPP_ */
