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
