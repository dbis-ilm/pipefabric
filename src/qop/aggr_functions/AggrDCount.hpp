/*
 * AggrDCount.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
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
