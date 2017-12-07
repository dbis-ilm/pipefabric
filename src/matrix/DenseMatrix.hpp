#ifndef DENSEMATRIX_HH
#define DENSEMATRIX_HH

#include <sstream>

#include "BaseMatrix.hpp"
#include "ReaderValue.hpp"
#include <eigen3/Eigen/Dense>

#include <boost/uuid/uuid.hpp>

namespace pfabric
{

	template<typename CellType, int Rows, int Cols>
	class DenseMatrix;


	template<typename M>
	struct DenseIterator : std::iterator<std::forward_iterator_tag, typename M::Scalar>
	{
		typedef DenseIterator<M> self_type;
		typedef typename M::Index IndexType;
		typedef const typename M::Scalar& reference;
		DenseIterator()
		: matrix(nullptr)
		, row(0)
		, col(0)
		, rows(0)
		, cols(0)
		{}

		DenseIterator(M *matrix, IndexType i, IndexType j, IndexType rows, IndexType cols)
		: matrix(matrix)
		, row(i)
		, col(j)
		, rows(rows)
		, cols(cols)
		{}

		DenseIterator(const self_type &rhs)
		: matrix(rhs.matrix)
		, row(rhs.row)
		, col(rhs.col)
		, rows(rhs.rows)
		, cols(rhs.cols)
		{}

		reference
		operator*() {
			return (*matrix)(row, col);
		}

		bool operator==(const self_type &rhs) const
		{
			return row == rhs.row && col == rhs.col;
		}
		bool operator!=(const self_type &rhs) const
		{
			return !operator==(rhs);
		}

		bool operator!() const 
		{
			return row == rows && col == cols;
		}

		self_type& operator++() {
			incIters();
			return *this;
		}

		self_type operator++(int) {
			auto tmp = *this;
			incIters();
			return tmp;
		}

		self_type& operator--() {
			decIters();
			return *this;
		}
		self_type operator--(int) {
			auto tmp = *this;
			decIters();
			return tmp;
		}

		IndexType getRow() const { return row; }
		IndexType getCol() const { return col; }

	private:

		M *matrix;
		IndexType row, col;
		const IndexType rows, cols;

		void incIters() {
			++row;
			if(row >= rows) {				
				++col;
				if (col < cols) row = 0;
			}
		}

		void decIters() {
			--row;
			if(row < 0) {				
				--col;
				if(col >= 0) row = rows-1;
			} 
		}

	};
	

	/** 
	* 
	* @brief The class uses dense matrix from Eigen library
	* 	and implements wrapped member functions to operate on the matrix
	* @tparam CellType
	*	the type determines scalar type of the matrix
	* @param Rows
	* 	the number of rows of the matrix which could be allocated at compile time
	*	If it is -1 than rows would be allocated at runtime
	* @param Cols
	*	the number of cols of the matrix  which could be allocated at compile time
	*	If it is -1 than cols would be allocated at runtime
	**/
	
	template<typename CellType, int Rows = Eigen::Dynamic, int Cols = Eigen::Dynamic>
	class DenseMatrix : public BaseMatrix
	{
	public:
		typedef CellType 									element_type;		//< type of cell of the matrix
		typedef Eigen::Index 								IndexType;			//< index type for traverse the matrix 
		typedef Eigen::Matrix<CellType, Rows, Cols> 		MatrixType;			//< typedef of the matrix
		typedef DenseMatrix<CellType, Rows, Cols> 			self_type;			//< typedef of the class
		typedef DenseIterator<const MatrixType>				const_iterator;		//< const iterator traverses over matrix
		typedef std::tuple<IndexType, IndexType, CellType>	Triplet;			//< for incidents values
		typedef boost::uuids::uuid 							Identifier; 		//< Type for unique identifier

		DenseMatrix() 
		{
		}
		DenseMatrix(IndexType rows, IndexType cols)
		: matrix(rows, cols)
		{}

		DenseMatrix(const MatrixType &mat)
		: matrix(mat)
		{}
		DenseMatrix(MatrixType && mat)
		: matrix(std::move(mat))
		{}

		DenseMatrix(self_type &&rhs)
		: matrix(std::move(rhs.matrix))
		, id(rhs.id)
		, incidentIndexes(std::move(rhs.incidentIndexes))
		{}

		DenseMatrix(const self_type &rhs)
		: matrix(rhs.matrix)
		, id(rhs.id)
		, incidentIndexes(rhs.incidentIndexes)
		{
			
		}

		~DenseMatrix(){}

		const MatrixType &getMatrix() const { return matrix; }

		inline
		void setMatrix(const MatrixType& m) { matrix = m; }

		/**
		* @brief setID
		*	the method to set new identifier
		* @param[in] identifier
		*	the new unique id for sliced part
		**/
		inline
		void setID(Identifier identifier) { id = identifier; }

		/**
		* @brief getID
		*	the method return unique identifier
		* @return identifier
		**/
		inline
		Identifier getID() const { return id; }
		

		void remove(IndexType x, IndexType y) 
		{
			assert(x >= 0 && y >= 0);
			assert(x < matrix.rows() && y < matrix.cols());

			matrix(x, y) = 0;
		}
		
		inline
		void resize(IndexType newRows, IndexType newCols)
		{
			matrix.conservativeResize(newRows, newCols);
		}
		inline
		void set(IndexType x, IndexType y, CellType value) { 
			
			auto resizeRow = x; auto resizeCol = y;	

			if(resizeRow >= this->getRows() || resizeCol >= this->getCols()) {
				
				if(resizeRow >= this->getRows()) {
					resizeRow += 1;
				}
				else { resizeRow = this->getRows();}
				
				if(resizeCol >= this->getCols()) {
					resizeCol += 1;
				}
				else {resizeCol = this->getCols();}
				
				resize(resizeRow, resizeCol);
				
			}			
			assert(x < this->getRows()); assert(y < this->getCols());
			matrix(x, y) = value;
		}

		inline 
		CellType& get(IndexType x, IndexType y) {
			return matrix(x, y);
		}
		inline
		CellType get(IndexType x, IndexType y) const {
			return matrix(x, y);
		}
		inline
		CellType& operator()(IndexType x, IndexType y) {
			return get(x, y);
		}
		inline
		CellType operator()(IndexType x, IndexType y) const {
			return get(x, y);
		}
		inline
		CellType* getRawData() {
			return matrix.data();
		}
		inline
		const CellType* getRawData() const {
			return matrix.data();
		}

		inline
		IndexType getRows() const { return matrix.rows(); }
		
		inline
		IndexType getCols() const { return matrix.cols(); }
		
		template <int R, int C>
		void insertRow(IndexType row, const DenseMatrix<CellType, R, C>& other)
		{
			assert(row >= 0);
			if(row > matrix.rows() || other.getRows() <= 0 || other.getCols() <= 0) return;

			MatrixType temp = matrix;
			auto resizeRows = other.getRows();
			auto shiftedRows = matrix.rows() - row;
			
			resize(matrix.rows()+resizeRows, other.getCols());

			matrix.bottomRows(shiftedRows) = matrix.block(row, 0, shiftedRows, matrix.cols());
			matrix.block(row, 0, resizeRows, other.getCols()) = other.getMatrix().block(0, 0, resizeRows, other.getCols());
		}

		template<int R, int C>
		void insertCol(IndexType col, const DenseMatrix<CellType, R, C> &other)
		{
			assert(col >= 0);
			if(col > matrix.cols() || other.getRows() <= 0 || other.getCols() <= 0) return;

			MatrixType temp = matrix;
			auto resizeCols = other.getCols();
			auto shiftedCols = matrix.cols() - col; 

			resize(other.getRows(), matrix.cols()+resizeCols);

			matrix.rightCols(shiftedCols) = matrix.block(0, col, matrix.rows(), shiftedCols);
			matrix.block(0, col, other.getRows(), resizeCols) = other.getMatrix().block(0, 0, other.getRows(), resizeCols);
 		}

		inline
		void removeRow(IndexType row)
		{
			assert(row >=0);
			if(row >= matrix.rows()) return;
			BaseMatrix::removeRow(matrix, row);
		}
		inline 
		void removeCol(IndexType col)
		{
			assert(col >= 0);
			if(col >= matrix.cols()) return;
			BaseMatrix::removeCol(matrix, col);			
		}
		self_type& operator=(const self_type& rhs)
		{
			matrix = rhs.matrix;
			return *this;
		}
		template<int R, int C>
		bool operator==(const DenseMatrix<CellType, R, C> &cmp) const
		{
			if (this->matrix.rows() != cmp.matrix.rows()
				|| this->matrix.cols() != cmp.matrix.cols() )
			{
				return false;
			}
			return this->matrix.isApprox(cmp.matrix);
		}

		const_iterator begin() const { return const_iterator(&matrix, 0, 0, matrix.rows(), matrix.cols()); }
		const_iterator end() const { return const_iterator(&matrix, matrix.rows(), matrix.cols(), matrix.rows(), matrix.cols()); }
	
		self_type& operator==(self_type&& rhs)
		{
			this->matrix = std::move(rhs.matrix);
			this->incidentIndexes = std::move(rhs.incidentIndexes);
			this->id = rhs.id;

			return *this;
		}
		/**
		* @brief add2end
		*	the method insert element at the last position of the matrix
		*	For example, it is a vector. Then it will insert to row/column (0, 1, 2, 3, 4 ...)
		*	according storage model
		*	If it column major matrix (3x3) like 	0 3 6 0 then it insert 9 to (3x3) position
		*	matrix becomes (4x4)					1 4 7 0
		*	and value appears at (3x3)				2 5 8 9 <-----
		*	
		* @param[in] value
		*	the new value to be added
		**/
		void add2end(CellType value)
		{
			IndexType i, j;
			if(MatrixType::Options == Eigen::ColMajor) {
				 i = matrix.rows();
				 j = matrix.cols() == 0 ? 0 : matrix.cols()-1;
			} else {
				i = matrix.rows() == 0 ? 0 : matrix.rows()-1;
				j = matrix.cols();
			}	
			this->set(i, j, value);			
		}
		/**
		* @brief addIncident
		*	Mainly, it is utilized for slice matrix
		*	It inserts value at the end of the matrix
		*	and store indexes of the value
		* @param[in] i
		*	the row of the value from matrix to be sliced
		* @param[in] j
		* 	the column of the value from matrix to be sliced
		* @param[in] value
		*	the value of the matrix should be stored
		**/
		void addIncident(IndexType i, IndexType j, CellType value)
		{
			add2end(value);
			incidentIndexes.push_back({i, j});
		}
		/**
		* @brief getIncident
		*	return value according to storage model
		*	If it is column major row will equal index
		*	and columnd will be last element if matrix is not empty
		*	Then it takes incidents indexes by index 
		* 	and finally, returns tuple {row, column, value}
		*
		* @param[in] index
		*	the index of incident indexes
		* @return tuple with {row, column, value}
		**/
		Triplet getIncident(IndexType index) const 
		{
			IndexType i, j;
			if(MatrixType::Options == Eigen::ColMajor) {
				i = index;
				j = matrix.cols() == 0 ? 0 : matrix.cols()-1;
			} else {
				i = matrix.rows() == 0 ? 0 : matrix.rows()-1;
				j = index;
			}
			auto value = matrix(i, j);
			auto ids = incidentIndexes[i];
			return std::make_tuple(ids.first, ids.second, value);
		}
		/**
		* @brief getCountIncidents
		*	The method is used for slicing matrix
		*	returns the number of incidents indexes
		* @return the number of incidents indexes
		**/
		std::size_t getCountIncidents() const { return incidentIndexes.size(); }
		 
		template<typename V, typename M>
		static M vector2matrix(const V &vector, size_t rows, size_t cols)
		{
			Eigen::Map<M> M1(vector.data(), rows, cols);
			return M1;
		}
		friend
		std::ostream& operator<<(std::ostream &stream, const self_type &matrix) {
			stream << matrix.getMatrix();
			return stream;
		}

	private:
		MatrixType matrix;
		Identifier id;
		std::vector<std::pair<IndexType, IndexType>> incidentIndexes;
	};

	/**
	* @brief The class supports all method of Dense class 
	*	and methods for stream like insert and erase a tuple
	*
	* @tparam CellType 
	*	the type determines scalar type of the matrix
	* @tparam Visitor
	*	the class defines the pattern of tuple elements and extracts them
	* @param Rows
	* 	the number of rows of the matrix which could be allocated at compile time
	*	If it is -1 than rows would be allocated at runtime
	* @param Cols
	*	the number of cols of the matrix  which could be allocated at compile time
	*	If it is -1 than cols would be allocated at runtime
	**/	
	template<typename CellType, typename Visitor, int Rows = Eigen::Dynamic, int Cols = Eigen::Dynamic>
	class DenseMatrixStream : public DenseMatrix <CellType, Rows, Cols> 
	{
	public:
		typedef typename Visitor::StreamElement StreamElement;		//< record type, TuplePtr< int, int, double >

		void erase(const StreamElement &rec)
		{
			Visitor v; v.erase(rec, this);
		}
		void insert(const StreamElement &rec) {	
			Visitor v; v.insert(rec, this);
		}
	};
	template<typename CellType, int Rows = Eigen::Dynamic>
	using VectorX = DenseMatrix<CellType, Rows, 1>;

	template<typename CellType, int Cols = Eigen::Dynamic>
	using VectorY = DenseMatrix<CellType, 1, Cols>;

} //pfabric

template<typename CellType, typename Visitor, int Rows = Eigen::Dynamic, int Cols = Eigen::Dynamic>
std::ostream & operator<<(std::ostream &stream, const pfabric::DenseMatrixStream<CellType, Visitor, Rows, Cols> &m )
{
	stream << m.getMatrix();
	return stream;
}

#endif //DENSEMATRIX_HH
	