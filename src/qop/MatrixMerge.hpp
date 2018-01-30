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

#ifndef MATRIXMERGE_HH
#define MATRIXMERGE_HH

#include "qop/UnaryTransform.hpp"
#include "qop/Queue.hpp"
#include "matrix/Matrix.hpp"

#include <unordered_map>

namespace pfabric
{
    /**
    * @brief MatrixMerge
    *   the operator joins partitions into a single complete matrix
    * @tparam StreamElement
    *   the partitions type (e.g. Sparse, Dense)
    **/

	template<typename StreamElement>
	class MatrixMerge : public UnaryTransform<StreamElement, StreamElement>
	{
	public:
		PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);
        // it is needed to obtain the partitions type which comes in tuples
		typedef typename StreamElement::element_type::template getAttributeType<0>::type   MatrixType;
        typedef typename MatrixType::Identifier PartitionID; //< the type of unique identifier

        /**
        * @brief MatrixMerge
        *   the constructor to initialize the number of partitions part
        * @param[in]
        *   the number of the parts
        **/
		MatrixMerge(std::size_t numParts) 
		: numParts(numParts)
		{}

		const std::string opName() const override { return std::string("MatrixMerge"); }

		/**
    	 * @brief Bind the callback for the data channel.
    	 */
    	BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, MatrixMerge, processDataElement);

    	/**
    	 * @brief Bind the callback for the punctuation channel.
    	 */
    	BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, MatrixMerge, processPunctuation);
		
       

    private:

        /**
        *
        * @brief Partition
        *   the type for counts merged parts and the future complete matrix
        *
        **/
        struct Partition
        {
            std::size_t countParts = 0; //< The count 
            MatrixType matrix; //< The complete matrix
        };
        
        // The type of partitions storage where
        // The PartitionID is unique identifier
        // The Partition is structure to count the number of arrival parts and to store matrix
        // The boost::hash<PartitionID> computes hash value from PartitionID
        typedef std::unordered_map<PartitionID, Partition, boost::hash<PartitionID>> PartitionStorage;

        std::size_t numParts; //< the number of partitions 
        PartitionStorage partitions; //< the hast table to store matrices

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

        /**
        * @brief processDataElement
        *   the method receives pieces of a matrix as a tuple and joins into complete matrix
        *   Firstly, it gets partition ID and merges parts with the complete matrix
        *   Secondly, it sends to the next if the number of merged parts equals the overall number parts
        *
        * @param[in] tuple
        *   the item carries matrix
        * @param[in] outdated
        *   the flag indicate if the data is recent or not
        **/
        void processDataElement(const StreamElement& tuple, const bool outdated) {

            const auto & mat = get<0>(tuple); // Obtain a matrix
            auto id = mat.getID(); // Obtain unique identifier
            auto& part = partitions[id]; // Obtain part or create new one
            merge_to(mat, part.matrix); // Merge piece to the matrix

            if(++part.countParts == numParts) {
                auto completeMat = makeTuplePtr(std::move(part.matrix)); // create TuplePtr to send it 
                this->getOutputDataChannel().publish(completeMat, outdated);
                partitions.erase(id); // Remove the entity from the storage
            }
        }

        /**
        * @brief merge_to
        *   the method merges one matrix to the complete matrix
        *   Mainly, it traverses over list of original indexes and values of the vector
        *   getting row and column it inserts the element to the big matrix
        *   
        * @tparam CellType
        *   the value type of the matrix
        * @tparam Rows
        *   the number of rows of the matrix that was allocated at compile time
        * @tparam Cols
        *   the number of cols of the matrix that was allocated at compile time
        * @param[in] srcMat
        *   a one of the parts of the matrix
        * @param[in] dstMat
        *   the future complete matrix
        **/
        template<typename CellType, int Rows, int Cols>
        void merge_to(const DenseMatrix<CellType, Rows, Cols> &srcMat
                    , DenseMatrix<CellType, Rows, Cols> &dstMat)
        {
            for(auto i = 0u; i < srcMat.getCountIncidents(); ++i)
            {
                auto triple = srcMat.getIncident(i);
                dstMat.set(std::get<0>(triple), std::get<1>(triple), std::get<2>(triple));
            }
        }

        /**
        * @brief merge_to
        *   the method merges one part to the complete matrix
        *   Generally,  if the source matrix is empty yet, then it jush copies it
        *   Otherwise, it traverses over non-zeros elements to insert them into the big matrix
        *
        * @tparam CellType
        *   the value type of a dense matrix
        * @tparam Rows
        *   the number of rows of the matrix that was allocated at compile time
        * @tparam Cols
        *   the number of cols of the matrix that was allocated at compile time
        * @param[in] srcMat
        *   the a one of the parts of the matrix
        * @param[in] dstMat
        *   the future complete matrix
        **/
        template<typename CellType, typename ReaderValue>
        void merge_to(const Matrix<CellType, ReaderValue>& srcMat
                    , Matrix<CellType, ReaderValue> &dstMat)
        {
            if(dstMat.getNumElements() == 0) {
                dstMat.setMatrix(srcMat.getMatrix());
            } else {
        	    for(auto beg = srcMat.begin(), end = srcMat.end(); beg != end; ++beg)
        	    {
        	   	   auto i = beg.getRow(); auto j = beg.getCol();
        	   	   auto v = *beg;
                   dstMat.set(i, j, v);   
                }
            }
        }
	};
}

#endif //MATRIXMERGE_HH
