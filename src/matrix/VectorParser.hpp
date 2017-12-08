#ifndef VECTORPARSER_HH
#define VECTORPARSER_HH

#include "DenseMatrix.hpp"
#include "SparseMatrix.hpp"
#include <string>
#include <sstream>

namespace pfabric
{
	/**
	* @brief the class is used for parsing values from tuples
	*	in `StringAttributeParser` class
	*	It contains methods reads items from string
	**/
	class VectorParser
	{
	public:

		template<typename CellType>
		static
		void parse(
			const std::string& input
			, SparseVector<CellType> &vector
			)
		{
			typedef typename SparseVector<CellType>::Index IndexType;

			std::istringstream stream(input);

			IndexType i = 0; 

			stream.setf(std::ios_base::skipws);
			CellType v;
	
			vector.resize(50);
			while(!stream.eof()) {		

				stream >> v;
				if(stream.eof()) break;
				if(i >= vector.rows()) vector.resize(vector.rows()*1.5);
			    vector.coeffRef(i) = v;
				i++;
			}

		}

		template<typename CellType, int Rows, int Cols>
		static
		void parse(
			const std::string &input
			, DenseMatrix<CellType, Rows, Cols> &vector
			, typename std::enable_if<Rows == 1, int>::type* dummy = 0)
		{
			typedef DenseMatrix<CellType, Rows, Cols> 	MatrixType;
			typedef typename MatrixType::IndexType 		IndexType;
			
			std::istringstream stream(input);

			IndexType i = 0; 
			IndexType j = 0;

			stream.setf(std::ios_base::skipws);
			CellType v;
	
			while(!stream.eof()) {		

				stream >> v;
				if(stream.eof()) break;

			    vector.set(i, j, v);
				j++;
			}
		}

		template<typename CellType, int Rows, int Cols>
		static
		void parse(
			const std::string &input
			, DenseMatrix<CellType, Rows, Cols> &vector
			, typename std::enable_if<Cols == 1, int>::type* dummy = 0)
		{
			typedef DenseMatrix<CellType, Rows, Cols> 	MatrixType;
			typedef typename MatrixType::IndexType 		IndexType;
			
			std::istringstream stream(input);

			IndexType i = 0; 
			IndexType j = 0;

			stream.setf(std::ios_base::skipws);
			CellType v;
	
			while(!stream.eof()) {		

				stream >> v;
				if(stream.eof()) break;

			    vector.set(i, j, v);
				i++;
			}
		}
	};
}
#endif //VECTORPARSER_HH