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

#ifndef StatefulMap_hpp_
#define StatefulMap_hpp_

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

/**
 * @brief An operator implementing a map operation with state.
 *
 * A StatefulMap operator produces tuples according to a given map function by
 * incorporating a state which is modified inside the map function.
 *
 * @tparam InputStreamElement
 *    the data stream element type consumed by the map operator
 * @tparam OutputStreamElement
 *    the data stream element type produced by the map operator
 * @tparam StateRep
 *    the class for representing the state
 */
template<
	typename InputStreamElement,
	typename OutputStreamElement,
  typename StateRep
>
class StatefulMap :
	public UnaryTransform< InputStreamElement, OutputStreamElement > // use default unary transform
{
private:
	PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement)

public:
  typedef std::shared_ptr<StateRep> StateRepPtr;

	/**
	 * Typedef for a function pointer to a projection function.
	 */
	typedef std::function<OutputStreamElement (const InputStreamElement&, bool, StateRepPtr)> MapFunc;

	/**
	 * @brief Construct a new instance of the stateful map operator.
	 *
	 * Create a new StatefulMap operator for evaluating the map function
	 * on each incoming tuple by taking the state into account.
	 *
	 * @param pfun function pointer to a map function
	 */
	StatefulMap(MapFunc f) : mFunc(f), mState(std::make_shared<StateRep>()) {}

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, StatefulMap, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, StatefulMap, processPunctuation );

	const std::string opName() const override { return std::string("StatefulMap"); }

private:

	/**
	 * @brief This method is invoked when a punctuation arrives.
	 *
	 * It simply forwards the punctuation to the subscribers.
	 *
	 * @param[in] punctuation
	 *    the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		this->getOutputPunctuationChannel().publish(punctuation);
	}

	/**
	 * This method is invoked when a data stream element arrives.
	 *
	 * It applies the projection function and forwards the projected element to its subscribers.
	 *
	 * @param[in] data
	 *    the incoming stream element
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement( const InputStreamElement& data, const bool outdated ) {
		auto res = mFunc( data, outdated, mState );
		this->getOutputDataChannel().publish( res, outdated );
	}


	MapFunc mFunc;       //< function pointer to the projection function
  StateRepPtr mState;  //< pointer to the state object
};

} // namespace pfabric

#endif
