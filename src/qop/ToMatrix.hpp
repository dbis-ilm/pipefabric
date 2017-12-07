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