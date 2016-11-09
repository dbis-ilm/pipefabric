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
 
#ifndef AGGRDCOUNT_HPP_
#define AGGRDCOUNT_HPP_

#include "AggregateFunc.hpp"

#include <unordered_map>
#include <type_traits>


namespace pfabric {

/**
 * @brief A distinct counting aggregation function.
 *
 * @tparam Tin
 *    the type of the input argument
 * @tparam Tres
 *    the type of the result (must be convertible from a hash table size)
 */
template<
	typename Tin,
	typename Tres
>
class AggrDCount :
	public AggregateFunc< Tin, Tres >
{
private:

	/// the type used for implementing the counter
	typedef unsigned int Count;

	/// a hash table storing the counters for
	typedef std::unordered_map<Tin, Count> ValueCounters;

	/// the result will be the number of unique entries in the table, i.e., its size
	static_assert( std::is_convertible< typename ValueCounters::size_type, Tres >::value,
		"result type must be convertible from a hash table size"
	);

public:

    AggrDCount() {
        this->init();
    }

	virtual void init() override {
        dCountElements.clear();
    }

	virtual void iterate(Tin const& data, bool outdated = false) override {
        typename ValueCounters::iterator it = dCountElements.find(data);

        if(it != dCountElements.end()) {
            it->second = it->second + (outdated ? -1 : 1);
            if(it->second <= 0) {
                this->dCountElements.erase(it);
            }
        } else {
        	assert( outdated == false );
            this->dCountElements[data] = 1;
        }
    }

	virtual Tres value() override {
        return this->dCountElements.size();
    }

private:
	ValueCounters dCountElements;
};

} /* end namespace pquery */


#endif /* AGGRDCOUNT_HPP_ */
