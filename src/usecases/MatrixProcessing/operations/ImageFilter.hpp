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

#ifndef IMGFILTER_HH
#define IMGFILTER_HH

#include <fstream>
#include <sstream>
#include "qop/OperatorMacros.hpp"
#include "core/StreamElementTraits.hpp"

namespace pfabric
{
    /**
    * @brief this operator is used for image processing filters.
    *   It applies a filter on come image from tuple
    * @tparam StreamElement 
    *   the tuple brings dense vector with image pixels
    * @tparam Filter
    *   the class is a particular filter (e.g. Gaussian Blur, Smooth ...)
    **/
	template<typename StreamElement, typename Filter>
	class ImageFilter : public
						UnaryTransform<StreamElement, StreamElement>
	{
		PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);
	public:

        ImageFilter(Filter filter)
        : filter(filter)
        {}

		const std::string opName() const override { return std::string("ImageFilter"); }

		/**
    	 * @brief Bind the callback for the data channel.
    	 */
    	BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, ImageFilter, processDataElement);

    	/**
    	 * @brief Bind the callback for the punctuation channel.
    	 */
    	BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, ImageFilter, processPunctuation);
	private:

        Filter filter;

		/**
    	 * @brief This method is invoked when a punctuation arrives.
    	 *
    	 * It simply forwards the @c punctuation to the subscribers.
    	 *
    	 * @param[in] punctuation
    	 *    the incoming punctuation tuple
    	 */
    	void processPunctuation(const PunctuationPtr& punctuation) {
    	 
    		this->getOutputPunctuationChannel().publish(punctuation);
    	}
    	void processDataElement(const StreamElement& rec, const bool outdated) {
    		auto &vector = get<0>(rec);
    		filter.apply(vector.getRawData(), vector.getRows(), vector.getCols());
    		this->getOutputDataChannel().publish(rec, outdated);
    	}

    	
	};
}
#endif //IMGFILTER_HH
