/*
 * Copyright (c) 2014-18 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#include "catch.hpp"

#include "pfabric.hpp"
#include "matrix/Matrix.hpp"
#include "matrix/ReaderValue.hpp"

#include "qop/MatrixSlice.hpp"
#include "qop/MatrixMerge.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef int CellType;


TEST_CASE("Slice dense matrix", "[MatrixSliceTest][DenseMatrix]")
{
	typedef DenseMatrix< CellType >   MatrixType;
	typedef typename MatrixType::MatrixType DenseMatrix;
	typedef TuplePtr<MatrixType> InputType;

	auto size = 4, count = 100, parts = 2;
	std::vector<InputType> inputs(count); 

	for(auto i = 0; i < count; ++i) {
		inputs[i] = makeTuplePtr(MatrixType(DenseMatrix::Random(size, size)));
	}
	auto opSlice = std::make_shared<MatrixSlice<InputType>>([](CellType v, int i, int j){return i%2;}, parts);
	auto opMerge = std::make_shared<MatrixMerge<InputType>>(parts);
	auto mockup = std::make_shared<StreamMockup<InputType, InputType>>(inputs, inputs);

	CREATE_DATA_LINK(mockup, opSlice);
	CREATE_DATA_LINK(opSlice, opMerge);
	CREATE_DATA_LINK(opMerge, mockup);

	mockup->start();
	REQUIRE(mockup->numTuplesProcessed() == count);
}


TEST_CASE("Slice sparse matrix", "[MatrixSliceTest][SparseMatrix]")
{
	typedef TuplePtr<int, int, CellType> ReaderType;
	typedef Matrix<CellType, ReaderValue<ReaderType> > MatrixType;
	typedef MatrixType::MatrixType SparseMatrix;
	typedef TuplePtr<MatrixType> InputType;

	auto size = 50, count = 100, parts = 2;
	std::vector<InputType> inputs(count);
	
	auto randMat = [](auto size) {

		SparseMatrix spMat(size, size);
		for(auto j = 0; j < size; ++j)
			for(auto i  = 0; i < size; ++i)
				spMat.coeffRef(i, j) = std::rand() % (size-1) + 0;

		return spMat;
	};
	for(auto i = 0; i < count; ++i) {
		MatrixType matrix; matrix.setMatrix(randMat(size));
		inputs[i] = makeTuplePtr(std::move(matrix));
	}
	
	auto opSlice = std::make_shared<MatrixSlice<InputType>>([](CellType v, int i, int j){return i%2;}, parts);
	auto opMerge = std::make_shared<MatrixMerge<InputType>>(parts);
	auto mockup = std::make_shared<StreamMockup<InputType, InputType>>(inputs, inputs);

	CREATE_DATA_LINK(mockup, opSlice);
	CREATE_DATA_LINK(opSlice, opMerge);
	CREATE_DATA_LINK(opMerge, mockup);

	mockup->start();

	REQUIRE(mockup->numTuplesProcessed() == count);
}