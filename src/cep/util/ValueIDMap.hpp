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



#ifndef ValueIDMap_hpp_
#define ValueIDMap_hpp_
#include<boost/unordered_map.hpp>
#include <string>


namespace pfabric {

template<typename T>
class ValueIDMap {

public:
	typedef boost::unordered_map<int, T> valueMap;
	typedef typename valueMap::const_iterator MapConstIterator;

	/**
	 * A constructor
	 */
	ValueIDMap() {
		counter = 0 ;
	}
	/**
	 * A destructor
	 */
	virtual ~ValueIDMap() {
	}
	/**
	 * Output the information  into ostream object
	 * @param out The output stream handle.
	 */
	//virtual void print(ostream& out = cout) const=0;

	/**
	 * Get the ID of a given value.
	 * @param value The value given to find the ID
	 * @return The ID of the given value
	 */
	const int getID(const T& value) const;

	/**
	 * Get the value of a given ID.
	 * @param id The ID given to find the value
	 * @return The value of the given ID
	 */
	T getValue(const int id) const {
		if (valueID.find(id) != valueID.end()) {
			return this->valueID.at(id);
		}
	}

	/**
	 * Put the value at the end of this map.
	 * @param value The value to append to this map
	 * @param The ID of the given value
	 */
	void appendValue(const T& value, const int id) {
		if (valueID.find(id) == valueID.end()) {
			counter++;
			std::pair<int, T> hashed_event(id, value);
			this->valueID.insert(hashed_event);
		}
	}

	/**
	 * Clear this map
	 */
	void clear() {
		valueID.clear();
	}

	/**
	 * Equality comparison operator.
	 * @param given_map The input map for this map to compare to
	 * @return True iff this map has the same sequence of values as that of given_map
	 */
	bool operator ==(const ValueIDMap<T>& given_map) const {
		if (valueID != given_map.valueID) {
			return false;
		}
		return true;
	}

	/**
	 * Inequality comparison operator.
	 * @param given_map The input map for this map to compare to
	 * @return True iff this map does not have the same sequence of values as
	 that of given_map
	 */
	bool operator !=(const ValueIDMap<T>& given_map) const {
		return !(*this == given_map);
	}

	/**
	 * Get the size of this map
	 * @return the size of this map
	 */

	size_t size() const {
		return valueID.size();
	}

	/**
	 * remove a value from this map
	 * @param  value id to remove it from this map
	 */
	void removeValue(const int id) {
		this->valueID.erase(id);
	}

	/**
	 * returns an iterator for navigating over the matches.
	 *
	 * @return a constant iterator
	 */
	MapConstIterator beginIterator() const {
		return valueID.begin();
	}

	/**
	 * returns the end iterator for navigating over the matches.
	 *
	 * @return a constant iterator
	 */
	MapConstIterator endIterator() const {
		return valueID.end();
	}
	/**
	 *  get number of insertions
	 *  @return as above
	 */
	int get_num_insertions() const {
		return counter;
	}

protected:

	/**
	 * To store the map from values to IDs in this map.
	 */
	valueMap valueID;
private:
	/**
	 * To store number of insertions
	 */
	int counter;
};

}

/* namespace pfabric */
#endif /* ValueIDMap_hpp_ */
