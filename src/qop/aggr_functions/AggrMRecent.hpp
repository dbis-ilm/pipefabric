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
