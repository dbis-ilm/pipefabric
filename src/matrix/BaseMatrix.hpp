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

#ifndef BASEMTRIX_HH__
#define BASEMTRIX_HH__


namespace pfabric
{
	struct MatrixParams
	{
		enum ModificationMode {
			  Insert = 0 	//< tuple was insert
			, Update		//< cell was updated
			, Delete		//< value was deleted
		};
	};
	/*
	* Base class for other matrices classes
	*/
	class BaseMatrix
	{
	protected:


		BaseMatrix(){}
	public:
		virtual ~BaseMatrix(){}

		template<typename M, typename Index>
		static void
		removeRow(M &matrix, Index row) {
			M temp = matrix;
			matrix.resize(matrix.rows()-1, matrix.cols());
			auto bottomRows = (temp.rows()-row)-1;
			matrix.topRows(row) = temp.topRows(row);
			matrix.bottomRows(bottomRows) = temp.bottomRows(bottomRows);
		}

		template<typename M, typename Index>
		static void
		removeCol(M &matrix, Index col)
		{
			M temp=matrix;
	  		matrix.resize(matrix.rows(), matrix.cols()-1);
	  		auto rightColSize = (temp.cols()-col)-1;
	  		matrix.leftCols(col) = temp.leftCols(col);
	  		matrix.rightCols(rightColSize) = temp.rightCols(rightColSize);
		}
	};


	template<typename T>
	struct MatrixTraits
	{
		typedef typename T::element_type 									element_type;
		typedef typename T::IndexType										IndexType;
		typedef std::pair<typename T::IndexType, typename T::IndexType > 	edge;  //< indexes of a matrix
	};
}

#endif //BASEMTRIX_HH__
