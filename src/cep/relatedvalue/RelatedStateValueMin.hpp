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

#ifndef RelatedStateValueMin_hpp_
#define RelatedStateValueMin_hpp_

#include "RelatedStateValue.hpp"
namespace pfabric {
template <class Tin, class StorageType, class ResultType,  int Index>
class RelatedStateValueMin: public RelatedStateValue<Tin, StorageType, ResultType,Index> {
private:
	/**
	 * store sum value for this state for particular tuple attribute
	 */
	StorageType minValue;
public:
	/**
	 * Gets the current value
	 * @return the current value
	 */
	ResultType getValue() {
		return minValue ;
	}

	/**
	 * Updates the value
	 * @param e the newly selected event
	 */
	void updateValue(const typename RelatedStateValue<Tin, StorageType, ResultType,Index>::TinPtr& e) {
		ResultType temp = std::get<Index>(e);
			if (temp < minValue) {
				this->minValue = temp;
			}
	}
	/**
	 * initializes the value by an event
	 * @param e
	 */
	void initValue(const typename RelatedStateValue<Tin, StorageType, ResultType,Index>::TinPtr& e) {
		updateValue(e);
	}
	/**
	 * constructor
	 */
	RelatedStateValueMin() {
		minValue =  std::numeric_limits<double>::max();
	}
	/**
	 * destructor
	 */
	virtual ~RelatedStateValueMin() {

	}
};
}
#endif /* RelatedStateValueMin_hpp_ */
