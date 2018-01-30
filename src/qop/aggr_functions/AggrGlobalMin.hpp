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
