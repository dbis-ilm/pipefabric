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

#ifndef SPARSEMATRIX_HH
#define SPARSEMATRIX_HH

#include <sstream>

#include "BaseMatrix.hpp"
#include "core/StreamElementTraits.hpp"
#include "ReaderValue.hpp"
#include <eigen3/Eigen/Sparse>

#include <mutex>
#include <boost/signals2.hpp>
#include <boost/uuid/uuid.hpp>

namespace pfabric{

	namespace MatrixDetails {

		enum class MatrixStructure  {
			Sparse,
			Dense
		};

		enum class MatrixOrder {
 			ColMajor,
 			RowMajor
		};
	}
	template< typename CellType, typename Visitor >
	class Matrix;

	template<typename Matrix, typename T>
	struct SparseEdgeIterator : public std::iterator<
	                       std::forward_iterator_tag,   // iterator_category
	                       T
	                       >
	{
	public:
		typedef SparseEdgeIterator<Matrix, T> 	self_type;
		typedef T 								edge_descriptor;

		SparseEdgeIterator()
		: index(0)
		, matrix_ptr(nullptr)
		{}

		SparseEdgeIterator(const Matrix *v, typename Matrix::IndexType id )
		: index(id)
		, matrix_ptr(v)
		, iterator(matrix_ptr->getMatrix(), index)
		{
		}

		SparseEdgeIterator(const self_type &rhs)
		: index(rhs.index)
		, matrix_ptr(rhs.matrix_ptr)
		, iterator(rhs.iterator)
		{}

		bool operator==(const self_type &rhs) {
			if(this->index != rhs.index) {
				while(!this->iterator && this->index != rhs.index) {
					++this->index;
					setIterator();
				}
				return !this->iterator;
			} else {
				return !this->iterator;
			}
		}

		bool operator!=(const self_type &rhs) {
			return !operator==(rhs);
		}

		self_type& operator++() {
			++iterator;
			return *this;
		}

		self_type& operator--() {
			--iterator;
			return *this;
		}

		self_type operator++(int) { auto tmp = *this; ++iterator; return tmp; }
		self_type operator--(int) { auto tmp = *this; --iterator; return tmp; }

		typename Matrix::IndexType
		getIndex() const {
			return this->iterator.index();
		}
		T operator*() const
		{
			return {this->iterator.col(), this->iterator.row()};
		}

	protected:
		typedef typename Matrix::MatrixType 		SpMatrix;
		typedef typename SpMatrix::InnerIterator 	EdgeMatIterator;

		typename Matrix::IndexType index;

		const Matrix *matrix_ptr;
		EdgeMatIterator iterator;

		void setIterator()
		{
			iterator = EdgeMatIterator(matrix_ptr->getMatrix(), index);
		}

	};

	template<typename Matrix, typename T>
	struct SparseInEdgeIterator : public SparseEdgeIterator < Matrix, T >
	{
		typedef SparseInEdgeIterator<Matrix, T> 	self_type;
		typedef T 								edge_descriptor;
		typedef std::ptrdiff_t					difference_type;
		typedef std::forward_iterator_tag  		iterator_category;

		SparseInEdgeIterator()
		{
			SparseEdgeIterator<Matrix, T>();
		}
		SparseInEdgeIterator(Matrix *m, typename Matrix::IndexType id)
		{
			SparseEdgeIterator<Matrix, T>(m, id);
		}
		SparseInEdgeIterator(const self_type &rhs)
		{
			SparseEdgeIterator<Matrix, T> ((const SparseEdgeIterator<Matrix, T>&) rhs);
		}

		T operator*() const
		{
			return { this->iterator.row(), this->iterator.col() };
		}
	};



	template<typename Matrix, typename T>
	struct SparseAdjVerticesIterator : public SparseEdgeIterator < Matrix, T >
    {
    	typedef SparseAdjVerticesIterator<Matrix, T> 	self_type;
    	typedef T 										edge_descriptor;

    	SparseAdjVerticesIterator()
    	{
    		SparseEdgeIterator<Matrix, T>();
    	}
    	SparseAdjVerticesIterator(Matrix *m, typename Matrix::IndexType id)
    	{
    		SparseEdgeIterator<Matrix, T>(m, id);
    	}

    	SparseAdjVerticesIterator(const self_type &rhs)
    	{
    		SparseEdgeIterator<Matrix, T>((const SparseEdgeIterator<Matrix, T>&) rhs);
    	}

    	T operator*() const
    	{
    		return this->iterator.row();
    	}
    };

    template<typename Matrix>
    struct SparseIterator : public SparseEdgeIterator< Matrix, typename Matrix::element_type >
    {
    	typedef SparseIterator<Matrix> self_type;
    	typedef typename Matrix::element_type Type;
    	typedef typename Matrix::element_type& reference;

    	SparseIterator()
    	: SparseEdgeIterator<Matrix, Type>()
    	{
    	}
    	SparseIterator(const Matrix *m, typename Matrix::IndexType id)
    	: SparseEdgeIterator<Matrix, Type>(m, id)
    	{
    	}
    	SparseIterator(const self_type& rhs)
    	: SparseEdgeIterator<Matrix, Type>((const SparseEdgeIterator<Matrix, Type>&) rhs)
    	{
    	}

    	typename Matrix::IndexType
    	getRow() const { return this->iterator.row(); }

    	typename Matrix::IndexType
    	getCol() const { return this->iterator.col(); }

    	Type operator*() const
    	{
    		return this->iterator.value();
    	}
    };

    /**
	* @brief Matrix
	*	uses Eigen sparse matrix
	* @tparam CellType
	*	the value type of the matrix
	* @tparam Visitor
	*	the tool to read values from a tuple
    **/

	template<typename CellType, typename Visitor>
	class Matrix : public BaseMatrix
	{
	public:
		typedef CellType 							element_type;		//< type of cell of the matrix
		typedef Eigen::Index						IndexType;			//< index type for traverse the matrix
		typedef typename Visitor::StreamElement		StreamElement;		//< record type, TuplePtr< int, int, double >
		typedef Eigen::SparseMatrix<CellType>	 	MatrixType;			//< typedef of the matrix
		typedef Matrix<CellType, Visitor>			self_type;			//< typedef of the class

		typedef SparseEdgeIterator<self_type, typename MatrixTraits<self_type>::edge > EdgeIterator;
		typedef SparseInEdgeIterator<self_type, typename MatrixTraits<self_type>::edge > InEdgeIterator;
		typedef SparseAdjVerticesIterator<self_type, element_type > AdjacentVertexIterator;
		typedef SparseIterator<self_type> iterator;

		//< callback is involved when the matrix was updated
  		typedef boost::signals2::signal<void (const StreamElement&, MatrixParams::ModificationMode)> ObserverCallback;
		typedef boost::uuids::uuid Identifier; //< Type for unique identifier

		Matrix() {}

		Matrix(IndexType rows, IndexType cols)
		: matrix(rows, cols)
		{}

		Matrix(self_type &&rhs)
		{
			std::lock_guard<std::mutex> g(rhs.mutex);
			matrix 		= std::move(rhs.matrix);
			observer	= std::move(rhs.observer);
		}
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

		/**
		* @brief set
		* 	the method insert new value by x and y coordinates
		*	If x/y is equal or greater than the current number
		*	of rows/columns of the matrix. Then, it will resize the matrix
		*
		* @param[in] x
		*	the x is a row
		* @param[in] y
		*	the y is a column
		* @param[in] value
		*	the new value should be inserted
		**/
		void set(IndexType x, IndexType y, CellType value)
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto resizeRow = x; auto resizeCol = y;

			if(resizeRow >= matrix.rows()) {
				resizeRow += 1;
			}
			else { resizeRow = matrix.rows();}

			if(resizeCol >= matrix.cols()) {
				resizeCol += 1;
			}
			else {resizeCol = matrix.cols();}

			resize(resizeRow, resizeCol);
			matrix.coeffRef(x, y) = value;

		}
		void insert(const StreamElement &rec )
		{
			Visitor v; v.insert(rec, this);
			observer(rec, MatrixParams::Insert);
		}
		void remove(IndexType x, IndexType y)
		{
			std::lock_guard<std::mutex> guard(mutex);
			matrix.coeffRef(x, y) = 0;
			preemt(0);
		}
		void preemt(CellType value) { matrix = matrix.pruned(value); }

		void erase(const StreamElement& rec)
		{
			Visitor v; v.erase(rec, this);
			observer(rec, MatrixParams::Delete);
		}

		void removeCol(IndexType col)
		{
			assert(col>=0);
			if(col >= matrix.cols()) return;
			BaseMatrix::removeCol(matrix, col);
		}

		void removeRow(IndexType row)
		{
			assert(row >= 0);
			if(row >= matrix.rows()) return;
			Eigen::SparseMatrix<CellType, Eigen::RowMajor> cpMat(matrix);
			BaseMatrix::removeRow(cpMat, row);
			matrix = cpMat;
		}

		inline
		CellType& get(IndexType x, IndexType y) {
			return matrix.coeffRef(x, y);
		}
		inline
		CellType get(IndexType x, IndexType y) const {
			return matrix.coeff(x, y);
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
		IndexType getRows() const { return matrix.rows(); }

		inline
		IndexType getCols() const { return matrix.cols(); }

		inline
		std::size_t getNumElements() const { return matrix.nonZeros(); }

		inline
		std::size_t getCountNonZerosByVer(IndexType index) const {

			auto id = matrix.outerIndexPtr()[index];
			IndexType end;
    		if(matrix.isCompressed())
    		  end = matrix.outerIndexPtr()[index+1];
    		else
    		  end = id + matrix.innerNonZeroPtr()[index];

    		return end - id;
		}

		inline
		const MatrixType & getMatrix() const {
			std::lock_guard<std::mutex> g(this->mutex);
			return matrix;
		}

		inline
		void setMatrix(const MatrixType& m) {
			std::lock_guard<std::mutex> g(this->mutex);
			matrix = m;
		}

		self_type& operator=(self_type &&rhs)
		{
			std::lock_guard<std::mutex> g(rhs.mutex);
			matrix 		= std::move(rhs.matrix);
			observer	= std::move(rhs.observer);

			return *this;
		}


		template<typename V>
		bool operator==(const Matrix<CellType, V> &cmp) const
		{
			std::lock_guard<std::mutex> g(cmp.mutex);
			if(	this->matrix.rows() != cmp.matrix.rows()
				|| this->matrix.cols() != cmp.matrix.cols())
			{
				return false;
			}
			return this->matrix.isApprox(cmp.matrix);
		}


		self_type& operator==(self_type &&rhs)
		{
			std::lock_guard<std::mutex> g(rhs.mutex);
			matrix 		= std::move(rhs.matrix);
			observer	= std::move(rhs.observer);

			return *this;
		}

		friend
		std::ostream & operator<<(std::ostream &stream, const self_type &m )
		{
			const MatrixType & matrix = m.getMatrix();
			stream << static_cast<const Eigen::SparseMatrixBase<MatrixType> & >(matrix);
			return stream;
		}

		void registerObserver(typename ObserverCallback::slot_type const& cb) {
			observer.connect(cb);
		}

		inline
		void resize(IndexType newRow, IndexType newCol)
		{
			if(newRow == matrix.rows() && newCol == matrix.cols()) return;

			matrix.conservativeResize(newRow, newCol);
		}

		inline
		iterator begin() const { return iterator(this, 0); }

		inline
		iterator end() const {

			return iterator(this, matrix.outerSize()-1);
		}
	private:
		Identifier			id;
		MatrixType 			matrix;
		ObserverCallback 	observer;
		mutable std::mutex 	mutex;
	};

	template<typename CellType>
	using SparseVector = Eigen::SparseVector<CellType>;
}

#endif //SPARSEMATRIX_HH
