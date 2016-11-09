/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
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
