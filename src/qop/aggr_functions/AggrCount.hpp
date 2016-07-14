/*
 * AggrCount.hpp
 *
 *  Created on: Feb 19, 2015
 *      Author: fbeier
 */

#ifndef AGGRCOUNT_HPP_
#define AGGRCOUNT_HPP_

#include "AggregateFunc.hpp"

#include <type_traits>


namespace pfabric {

/**
 * @brief A counting aggregation function.
 *
 * @tparam Tin
 *    the type of the input argument
 * @tparam Tres
 *    the type of the result (must be convertible from unsigned int)
 */
template<
	typename Tin,
	typename Tres
>
class AggrCount :
	public AggregateFunc< Tin, Tres >
{
private:

	/// the type used for implementing the counter
	typedef unsigned int Count;

	static_assert( std::is_convertible< Count, Tres >::value,
		"result type must be convertible from a counter"
	);

public:

	AggrCount() {
		init();
	}

	virtual void init() override {
		mCount = 0;
	}

	virtual void iterate(Tin const& data, bool outdated = false) override {
 		mCount += (outdated ? -1 : 1);
	}

	virtual Tres value() override {
		return mCount;
	}

private:
	Count mCount;
};

} /* end namespace pquery */


#endif /* AGGRCOUNT_HPP_ */
