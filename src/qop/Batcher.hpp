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

#ifndef Batcher_hpp_
#define Batcher_hpp_

#include <vector>

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

template <typename InputStreamElement>
using BatchPtr = TuplePtr<Tuple<std::vector<std::pair<InputStreamElement, bool>>>>;

template <typename InputStreamElement>
class Batcher : public UnaryTransform< InputStreamElement, BatchPtr<InputStreamElement> > // use default unary transform
{
typedef BatchPtr<InputStreamElement> OutputStreamElement;

private:
PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement)

public:
  Batcher(std::size_t batchSize) : mBatchSize(batchSize), mPos(0), mBuf(batchSize) {}

  /**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, Batcher, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, Batcher, processPunctuation );

	const std::string opName() const override { return std::string("Batcher"); }

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
	 * It ... and forwards the resulting element to its subscribers.
	 *
	 * @param[in] data
	 *    the incoming stream element
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement( const InputStreamElement& data, const bool outdated ) {
		mBuf[mPos++] = std::make_pair(data, outdated);
    if (mPos == mBatchSize) {
      auto tup = makeTuplePtr(std::move(mBuf));
      this->getOutputDataChannel().publish(tup, false);
      mPos = 0;
      mBuf.resize(mBatchSize);
    }
	}

  std::size_t mBatchSize, mPos;
  std::vector<std::pair<InputStreamElement, bool>> mBuf;
};

}

#endif
