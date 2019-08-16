/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef Merge_hpp_
#define Merge_hpp_

#include "qop/UnaryTransform.hpp"

namespace pfabric {

/**
 * @brief a streaming operator for merging multiple streams into a single one.
 *
 * Merge is an operator which subscribes to multiple streams and combines all tuples
 * produced by these input stream into a single stream and forwards the tuples
 * to the subscriber.
 *
 * @tparam StreamElement
 *   the data stream element type consumed by Merge
 */
 template<typename StreamElement>
class Merge : public UnaryTransform<StreamElement, StreamElement, true> {
private:
	PFABRIC_SYNC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, Merge, processDataElement );


	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, Merge, processPunctuation );

	/**
	 * @brief This method is invoked when a tuple arrives from the input channel.
	 * It simply forwards the punctuation to the subscribers.
	 *
	 * @param[in] data
	 *    the incoming stream element from the input channel
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement( const StreamElement& data, const bool outdated ) {
    this->getOutputDataChannel().publish( data, outdated );
	}

	/**
	 * @brief This method is invoked when a punctuation arrives.
	 *
	 * It simply forwards the punctuation to the subscribers.
	 *
	 * @param[in] punctuation
	 *    the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		this->getOutputPunctuationChannel().publish( punctuation );
	}
public:
	/**
	 * Creates a new operator instance and subscribes to the given
	 * source operator.
	 */
	Merge() {}

  const std::string opName() const override { return std::string("Merge"); }

};
}

#endif
