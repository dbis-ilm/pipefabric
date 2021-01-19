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

#ifndef TOMATRIX_HH
#define TOMATRIX_HH

#include <memory>

#include "qop/OperatorMacros.hpp"
#include "matrix/Matrix.hpp"

namespace pfabric {
// 
template<
	typename MatrixType
>
class ToMatrix : public UnaryTransform<typename MatrixType::StreamElement, typename MatrixType::StreamElement> {

private:
	PFABRIC_UNARY_TRANSFORM_TYPEDEFS(typename MatrixType::StreamElement, typename MatrixType::StreamElement);

	typedef std::shared_ptr<MatrixType>        MatrixPtr;
	typedef typename MatrixType::StreamElement StreamElement;
    
	MatrixPtr matrix;

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

    
    void processDataElement(const StreamElement& rec, const bool outdated) 
    {        
        if(outdated) {
            matrix->erase(rec);
        } else {
            matrix->insert(rec);
        }

        this->getOutputDataChannel().publish(rec, outdated);
    }
	

public:

	explicit ToMatrix(MatrixPtr ptr) : matrix(ptr) {}

	const std::string opName() const override { return std::string("ToMatrix"); }

	/**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, ToMatrix, processDataElement);

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, ToMatrix, processPunctuation);


};
}
#endif //TOMATRIX_HH
