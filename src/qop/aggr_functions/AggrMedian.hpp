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

 
#ifndef AGGRMEDIAN_HPP_
#define AGGRMEDIAN_HPP_

#include "AggregateFunc.hpp"

#include <map>
#include <type_traits>


namespace pfabric {

/**
 * @brief An aggregation function for calculating the median in a stream.
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
	typename Tres,
	class Comparator
>
class AggrMedian :
	public AggregateFunc< Tin, Tres >
{
private:

	/// the type used for implementing the counter
	typedef unsigned int Count;

	/// an ordered map for storing counters for each element
	typedef std::map<Tin, Count, Comparator> ValueCounters;

	/// the result must be convertible from a floating point type
	static_assert( std::is_convertible< double, Tres >::value
			|| std::is_convertible< float, Tres >::value,
		"result type must be convertible from a hash table size"
	);


public:

    AggrMedian(){
        this->init();
    }

	virtual void init() override {
        this->total = 0;
        this->mapElement.clear();
        this->mapPos.elemBefore = 0;
        this->mapPos.mapIt = this->mapElement.end();
    }

	virtual void iterate(Tin const& data, bool outdated = false) override {
        typename ValueCounters::iterator it;
        Comparator cmp; // TODO really need to create an instance here?

        //search element
        it = this->mapElement.find(data);
        if(outdated) {
            if(it != this->mapElement.end()) {
                //Element exists

                if(it == this->mapPos.mapIt) {
                    if(this->total % 2 != 0) {
                        this->decrementMap();
                    } else if(this->total % 2 == 0 &&
                            this->mapPos.elemBefore+1 >= this->mapPos.mapIt->second) {
                        this->incrementMap();
                    }
                } else if(this->total % 2 != 0 &&
                        !cmp(data, this->mapPos.mapIt->first)) {
                    this->decrementMap();
                } else if(this->total % 2 == 0 &&
                         cmp(data, this->mapPos.mapIt->first)) {
                    this->incrementMap();
                }

                //Modify map
                if(it->second == 1) {
                    //remove
                    this->mapElement.erase(it);
                    this->total--;

                    if(this->total == 0) {
                        this->mapPos.mapIt = this->mapElement.end();
                        this->mapPos.elemBefore = 0;
                        return;
                    }
                } else {
                    it->second--;
                    this->total--;
                }
            }
        } else {
            //Modify map
            if(it != this->mapElement.end()) {
                //element found
                it->second++;
                this->total++;
            } else {
                //new element
                this->mapElement.insert(std::pair<Tin, long>(data, 1));
                this->total++;

                if(this->total == 1) {
                    //Very first element
                    this->mapPos.mapIt = this->mapElement.begin();
                    this->mapPos.elemBefore = 0;
                    return;
                }
            }

            if(this->total % 2 == 0 &&
                     cmp(data, this->mapPos.mapIt->first)) {
                //change iterator element
                this->decrementMap();
            } else if(this->total % 2 != 0 &&
                    !cmp(data, this->mapPos.mapIt->first)) {
                //element was not found and is greater or equal
                this->incrementMap();
            }
        }
    }

	virtual Tres value() override {
        if(this->total == 0) {
            return 0.0;
        } else if(this->total % 2 == 0) {
            Tin currElement, nextElement;

            currElement = this->mapPos.mapIt->first;
            if(this->mapPos.elemBefore+1 >= this->mapPos.mapIt->second) {
                //fetch next element
            	this->mapPos.mapIt++;
                //get next element and go the previous one
                nextElement = this->mapPos.mapIt->first;
                //move back
                this->mapPos.mapIt--;
                //calculate median
                return (currElement + nextElement)/2.0;
            } else {
                //-> Does not move to another position
                return currElement;
            }
        } else {
            return this->mapPos.mapIt->first/1.0;
        }
    }

private:
    //total counter of all elements
    long total;

    ValueCounters mapElement;

    struct MedianPosition {
        //current iterator position
    typename ValueCounters::iterator mapIt;
        //keeps the number of elements, which are saved before this->mapPos.mapIt
        int elemBefore;
    } mapPos;


    void decrementMap() {
        if(this->mapPos.elemBefore <= 0) {
            this->mapPos.mapIt--;
            this->mapPos.elemBefore = this->mapPos.mapIt->second-1;
        } else {
        	this->mapPos.mapIt--;
        }
    }
    void incrementMap() {
        if(this->mapPos.elemBefore+1 >= this->mapPos.mapIt->second) {
        	this->mapPos.mapIt++;
            this->mapPos.elemBefore = 0;
        } else {
        	this->mapPos.mapIt++;
        }
    }
};

} /* end namespace pquery */



#endif /* AGGRMEDIAN_HPP_ */
