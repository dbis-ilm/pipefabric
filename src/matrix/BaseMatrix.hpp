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