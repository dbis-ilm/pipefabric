/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
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
