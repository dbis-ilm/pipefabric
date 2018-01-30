/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
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
