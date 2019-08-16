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

#ifndef MATRIXSLICE_HH
#define MATRIXSLICE_HH

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

#include "matrix/Matrix.hpp"
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
namespace pfabric
{

	/**
	* @brief MatrixSlice
    *   The operator decouples matrix into parts using user defined function
	* @tparam StreamElement
	*	the type of matrix (e.g. Dense, Sparse)
	**/
	template<typename StreamElement >
	class MatrixSlice : public UnaryTransform<StreamElement, StreamElement> 
	{
    public:
		PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);
        
        // It is needed to obtain type of income matrix
        typedef typename StreamElement::element_type::template getAttributeType<0>::type   MatrixType; 
        
        typedef typename MatrixType::IndexType      Index;      //< index type of the matrix
        typedef typename MatrixType::element_type   MatElement; //< value type of the matrix
        
        /**
        * @brief Predicate holds UDF to determine ID of storage to insert value
        * @param[in] MatElement
        *   the value of the matrix
        * @param[in] Index
        *   the row index of the matrix
        * @param[in] Index
        *   the col index of the matrix
        * @return the index of the partition
        **/
        typedef std::function<std::size_t (MatElement, Index, Index)> Predicate;

        /**
        * @brief MatrixSlice
        *   the constructor to initialize slice function and the number of partitions
        * @param[in] predicate
        *   the slice function is defined by a user
        * @param[in] numParts
        *   the number of partitions
        **/
		MatrixSlice(Predicate predicate, std::size_t numParts)
		: sliceFun(predicate)
		, numParts(numParts)
		{}

		const std::string opName() const override { return std::string("MatrixSlice"); }

		/**
    	 * @brief Bind the callback for the data channel.
    	 */
    	BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, MatrixSlice, processDataElement);

    	/**
    	 * @brief Bind the callback for the punctuation channel.
    	 */
    	BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, MatrixSlice, processPunctuation);
		


    private:
        typedef std::vector<MatrixType>             Partitions; //< container type to store parts of a matrix
        typedef typename MatrixType::Identifier     PartitionID; //< type to uniquely label

        Predicate sliceFun;     //< slice function  
        std::size_t numParts;   //< number of partitions

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
        *   receives tuples which contains a matrix (e.g. sparse and dense matrices)
        *   and splits the matrix into pieces
        *   For that there are two methods for both sparse and dense matrices.
        * @param[in] rec
        *   the tuple contains either dense or sparse matrix
        * @param[in] outdated
        *   the flag shows if tuple is recent or not
        **/
        void processDataElement(const StreamElement& rec, const bool outdated) 
        {        
            Partitions partitions;
            // Chose implementation according the type of matrix
            slice(partitions, get<0>(rec));

            // Generate new unique ID for all parts
            auto partID = generatePartitionID();
            for(auto i = 0u; i < numParts; ++i) {
                partitions[i].setID(partID);
            }
            // Send to the downstream all parts
            for(auto &m : partitions) {                
                this->getOutputDataChannel().publish(makeTuplePtr(std::move(m)), false);
            }
        }
        /**
        * @brief The function decouples a dense matrix into several parts
        *   creating new matrices. 
        *   The matrix is placed within income tuple 
        *   Sliced matrix is represented as a vector. Because it needs to keep 
        *   only non-zeros elements avoiding waste of spase of memory
        *   To reconstruct complete matrix from pieces it has to hold indexes (original place)
        *   from source matrix.
        *
        * @tparam CellType
        *   the value type of a dense matrix
        * @tparam Rows
        *   the number of rows of the matrix that was allocated at compile time
        * @tparam Cols
        *   the number of cols of the matrix that was allocated at compile time
        * @param[in] partitions
        *   a storage holds separated parts of the source matrix
        * @param[in] matrix
        *   the source matrix which should be partitioned
        **/

        template<typename CellType, int Rows, int Cols>  
        void slice(Partitions& partitions, const DenseMatrix<CellType, Rows, Cols>& matrix)
        {
            partitions.resize(numParts);
            for(auto beg_it = matrix.begin(), end_it = matrix.end(); beg_it != end_it; ++beg_it)
            {
                auto i = beg_it.getRow(); auto j = beg_it.getCol();
                auto v = *beg_it;
                auto id = sliceFun(v, i, j);

                partitions[id].addIncident(i, j, v);
            }             
        }

        /** 
        * @brief The function decouples a sparse matrix into several parts
        *   creating new matrices. The matrix is placed within income tuple.
        *   At the beginning parts are created with the size of source matrix
        *   to avoid unnecessary realocate.
        *   Then, it inserts values with original indexes from the matrix,
        *   calling set method
        *
        * @tparam CellType
        *   the value type of a sparse matrix
        * @tparam ReaderValue
        *   the class is responsible for extracting values from a tuple
        * @param[in] partitions
        *   a storage holds separated parts of the source matrix
        * @param[in] matrix
        *   the source matrix which should be partitioned
        **/
        template<typename CellType, typename ReaderValue> 
        void slice(Partitions& partitions, const Matrix<CellType, ReaderValue>& matrix)
        {   
            partitions.reserve(numParts);
            for(auto i = 0u; i < numParts; ++i) {
                partitions.emplace_back(matrix.getRows(), matrix.getCols());
            }           
            for(auto beg_it = matrix.begin(), end_it = matrix.end(); beg_it != end_it; ++beg_it)
            {
                auto i = beg_it.getRow(); auto j = beg_it.getCol();
                auto v = *beg_it;
                auto id = sliceFun(v, i, j);
                
                partitions[id].set(i, j,  v);
            }
        }
        /**
        * @brief generatePartitionID
        *   the method generates new partition ID for the parts of the matrix
        * @return new generated id which is UUID from the Boost Library
        **/
        static
        PartitionID generatePartitionID()
        {
            return boost::uuids::random_generator()();
        }
        

    };
}

#endif //MATRIXSLICE_HH
