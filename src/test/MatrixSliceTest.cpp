#define CATCH_CONFIG_MAIN

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