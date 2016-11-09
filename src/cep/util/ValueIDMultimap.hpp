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

#ifndef ValueIDMultimap_hpp_
#define ValueIDMultimap_hpp_
#include<boost/unordered_map.hpp>
#include "Partition.hpp"
using namespace std;

namespace pfabric {

/**
 * @brief A container to store a partition and its associated NFA structures which are
 * the candidates to be the complex event. This storage make the look-up more faster
 */
template<typename T, typename TinPtr>
class ValueIDMultimap {

public:
	typedef typename boost::unordered_multimap<Partition<TinPtr>*, T, hash_function<TinPtr>,
			equal_function<TinPtr>> ValueMultimap;
	typedef typename ValueMultimap::const_iterator MultimapConstIterator;
	typedef typename ValueMultimap::iterator MultimapIterator;
	typedef typename std::pair<MultimapIterator, MultimapIterator> MultimapPair;

	/**
	 * A constructor to init the counter
	 */
	ValueIDMultimap() {
		counter = 0;
	}
	/**
	 * A destructor
	 * Nothing to do
	 */
	virtual ~ValueIDMultimap() {
	}
	/**
	 * Output the information  into ostream object
	 * @param out The output stream handle.
	 */
	virtual void print(ostream& out = cout) const=0;

	/**
	 * Get the ID of a given value.
	 * @param value The value given to find the ID
	 * @return The ID of the given value
	 */
	const Partition<TinPtr>* getID(const T value) const;

	/**
	 * Get the value of a given ID.
	 * @param id The ID given to find the value
	 * @return The value of the given ID
	 */
	MultimapPair getValue(Partition<TinPtr>* id) {
		return valueID.equal_range(id);
	}
	/**
	 * Put the value at the end of this map.
	 * @param value The value to append to this map
	 * @param The ID of the given value
	 */
	void appendValue(Partition<TinPtr>* id, const T value) {
		valueID.insert(make_pair(id, value));
		counter++;
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
	bool operator ==(const ValueIDMultimap<T, TinPtr>& given_map) const {
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
	bool operator !=(const ValueIDMultimap<T, TinPtr>& given_map) const {
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
	void removeValue(
			typename boost::unordered_multimap<Partition<TinPtr>*, T>::iterator id) {
		this->valueID.erase(id);
	}

	/**
	 * remove a value from this map
	 * @param an iterator point to an element to remove from this map
	 */
	void removeValue(MultimapConstIterator it) {
		this->valueID.erase(it);
	}

	/**
	 * returns an iterator for navigating over the matches.
	 *
	 * @return a constant iterator
	 */
	MultimapConstIterator beginConstIterator() const {
		return valueID.begin();
	}

	/**
	 * returns the end iterator for navigating over the matches.
	 *
	 * @return a constant iterator
	 */
	MultimapConstIterator endConstIterator() const {
		return valueID.end();
	}

	/**
	 * returns an iterator for navigating over the matches.
	 *
	 * @return an iterator
	 */
	MultimapIterator beginIterator() const {
		return valueID.begin();
	}

	/**
	 * returns the end iterator for navigating over the matches.
	 *
	 * @return an iterator
	 */
	MultimapIterator endIterator() const {
		return valueID.end();
	}
	/**
	 *  get number of insertions
	 *  @return as above
	 */
	long getNumInsertions() const {
		return counter;
	}

	ValueMultimap& getMap() {
		return valueID;
	}
public:

	/**
	 * To store the map from values to IDs in this map.
	 */
	ValueMultimap valueID;
private:
	/**
	 * To store the number of insertions
	 */
	long counter;
};

}

/* namespace pfabric*/
#endif /* ValueIDMultimap_hpp_ */
