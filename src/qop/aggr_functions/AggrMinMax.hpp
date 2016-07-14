/*
 * AggrMinMax.hpp
 *
 *  Created on: Feb 20, 2015
 *      Author: fbeier
 */

#ifndef AGGRMINMAX_HPP_
#define AGGRMINMAX_HPP_

#include "AggregateFunc.hpp"

#include <map>
#include <utility>
#include <cassert>


namespace pfabric {

/**
 * @brief An aggregation function for calculating an extremum in a stream.
 *
 * @tparam Tin
 *    the type of the input argument
 * @tparam Tres
 *    the type of the result (must be convertible from float or double)
 * @tparam Comparator
 *    the comparator to be used for ordering elements
 */
template<
	typename Tin,
	class Comparator
>
class AggrMinMax :
	public AggregateFunc< Tin, Tin >
{
private:

	/// the type used for implementing the counter
	typedef unsigned int Count;

	/// an ordered map for storing counters for each element
	typedef std::map<Tin, Count, Comparator> ValueCounters;

public:

    AggrMinMax() {
        init();
    }

	virtual void init() override {
        mMap.clear();
    }

	virtual void iterate(Tin const& data, bool outdated = false) override {
        typename ValueCounters::iterator it;
        it = mMap.find(data);
        if (it != mMap.end()) {
            // element already exists
            // increment or decrement count based on outdated value
            it->second = it->second + (outdated ? -1 : 1);
            //Tuple value is fully outdated
            if (it->second <= 0) {
                //Remove
                mMap.erase(it);
            }
        }
        else {
            //Insert new element
            mMap.insert(std::make_pair(data, 1));
        }
    }

	virtual Tin value() override {
        typename ValueCounters::const_iterator it;
        it = mMap.begin();
        assert(it != mMap.end());
        return it->first;
    }

private:
	ValueCounters mMap;
};

} /* end namespace pquery */


#endif /* AGGRMINMAX_HPP_ */
