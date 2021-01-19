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

 
#ifndef AggrIdentity_hpp_
#define AggrIdentity_hpp_

#include "AggregateFunc.hpp"

#include <type_traits>


namespace pfabric {

/**
 * @brief A aggregation function that just keeps the last value.
 *
 * A aggregation function that keeps the last value which can be used
 * to store the grouping value.
 *
 * @tparam T
 *    the type of the input/output argument
 */
template<
	typename T
>
class AggrIdentity : public AggregateFunc< T, T > {
public:
	AggrIdentity() {
		init();
	}

	virtual void init() override {
	}

	virtual void iterate(T const& data, bool outdated = false) override {
 	  mValue = data;
	}

	virtual T value() override {
		return mValue;
	}

private:
  T mValue;
};

} 


#endif /* AggrIdentity_hpp_ */
