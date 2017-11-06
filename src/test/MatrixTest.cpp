#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <vector>

#include "pfabric.hpp"
#include "matrix/Matrix.hpp"
#include "matrix/ReaderValue.hpp"
#include "qop/ToMatrix.hpp"

#include "StreamMockup.hpp"


using namespace pfabric;

typedef int                                              CellType;
typedef TuplePtr<int, int, CellType>                     InputType;

StreamGenerator<InputType>::Generator stream([]( size_t n) -> InputType {
  int cols = 3;
  auto pos = std::div(n, cols);
  return makeTuplePtr((int)pos.rem, (int)pos.quot, (CellType)n);
});

TEST_CASE("Dense matrix. Insert operations", "[MatrixOperations]")
{

 typedef DenseMatrixStream<CellType, ReaderValue<InputType> >   MatrixType;

 std::vector<CellType> expected({0, 1, 2, 3, 4, 5, 6, 7, 8});

 auto matrix = std::make_shared<MatrixType>();
 Topology t;
 
 int sizeMatrix = 9;
 auto s = t.streamFromGenerator<InputType>(stream, sizeMatrix)
	.toMatrix< MatrixType >(matrix)
	;

 t.start(false); 

 auto data = matrix->getRawData();
 REQUIRE(matrix->getRows() == 3);
 REQUIRE(matrix->getCols() == 3);
 
 for(size_t i = 0, rows = matrix->getRows(); i < rows; ++i)
  for(size_t j = 0, cols = matrix->getCols(); j < cols; ++j) {
    auto id = i*rows + j;
    REQUIRE(data[id] == expected[id]);
  }
}

TEST_CASE("Filling random values", "[MatrixOperations][SparseMatrix]")
{
  typedef Matrix<CellType, ReaderValue<InputType> > MatrixType;

  const std::size_t size = 50;
  std::vector<InputType> inputs; inputs.reserve(size);

  std::srand(time(NULL));
  for(auto i = 0u; i < size; ++i)
  {
    int x = std::rand() % 100 + 0;
    int y = std::rand() % 100 + 0;
    CellType z = std::rand() % 50 + 0;
    inputs.push_back(makeTuplePtr(x, y, z));
  }
  std::vector<InputType> inputs2;
  auto matrix = make_shared<MatrixType>();

  auto mockup = std::make_shared<StreamMockup<InputType, InputType> >(inputs, inputs2);  
  
  auto op = std::make_shared< ToMatrix< MatrixType > > (matrix);

  CREATE_DATA_LINK(mockup, op);

  mockup->start();

  for(const auto &tuple : inputs)
  {
    auto x = get<0>(tuple);
    auto y = get<1>(tuple);
    auto z = get<2>(tuple);

    REQUIRE(matrix->get(x, y) == z);
  }
}

TEST_CASE("Read vector from a tuple to matrix", "[MatrixTest]")
{
  std::vector<CellType> inputs = {5, 7, 2, 5, 4, 8, 6, 2};
  std::string vec("5 7 2 5 4 8 6 2");
  std::vector<StringRef> valTuple; 
  valTuple.emplace_back("1", 1);
  valTuple.emplace_back("8", 1);
  valTuple.push_back(StringRef(vec.c_str(), vec.size()));

  auto validator = [&valTuple, &inputs](const auto& tuple){
    
  
    REQUIRE(get<0>(tuple) == 1);
    REQUIRE(get<1>(tuple) == 8);
  
    const auto& matrix = get<2>(tuple);
  
    for(size_t i = 0u, rows = matrix.getRows(); i < rows; ++i) 
      for(size_t j = 0u, cols = matrix.getCols(); j < cols; ++j){
        auto id = i*rows+j;
        REQUIRE(matrix.get(i, j) == inputs[id]);
    }
  };

  { // Sparse matrix
    typedef SparseVector<CellType>              VectorCol;
    typedef Tuple< int, int, VectorCol >       Tuple;
  
    Tuple record(valTuple.data());
    // validator(record);
  }

  { // Dense matrix

    // typedef DenseMatrix<CellType, ReaderValue<InputType> > MatrixType;
    typedef VectorY<CellType> VectorCol;
    typedef Tuple< int, int, VectorCol > Tuple;
    
    Tuple record(valTuple.data());
    validator(record);
  }
}

TEST_CASE("Resize dense matrix by inserting new tuples", "[MatrixOperations]")
{
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  const int sizeMatrix = 9;
  const std::vector<CellType>     expected({0, 1, 2, 3, 4, 5, 6, 7, 8});
  const std::vector< InputType >  extraVal = { makeTuplePtr(3, 0, 22), makeTuplePtr(3, 1, 33), makeTuplePtr(3, 2, 44) };

  auto matrix = std::make_shared< MatrixType >();
  auto opGenerator    = std::make_shared< StreamGenerator<InputType> >(stream, sizeMatrix);
  auto opToMatrix     = std::make_shared< ToMatrix< MatrixType > >(matrix);
  
  CREATE_DATA_LINK(opGenerator, opToMatrix);
  
  opGenerator->start();
  
      
  REQUIRE(matrix->getRows() == 3);
  REQUIRE(matrix->getCols() == 3);

  for(auto& tuple : extraVal) {
    matrix->insert(tuple);
  }

  REQUIRE(matrix->getCols() == 3);
  REQUIRE(matrix->getRows() == 4);

  for(size_t i = 0, rows = 3; i < rows; ++i)
    for(size_t j = 0, cols = 3; j < cols; ++j) {
      auto id = i*rows + j;
      REQUIRE(matrix->get(j, i) == expected[id]);
    }

  for(auto& tuple : extraVal) {
    auto x = tuple->getAttribute<0>();
    auto y = tuple->getAttribute<1>();
    auto v = tuple->getAttribute<2>();
    REQUIRE(matrix->get(x, y) == v);
  }
}


TEST_CASE("Resize dense matrix calling resize method directly", "[MatrixOperations]")
{
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  const int sizeMatrix = 9;
  const std::vector<CellType>     expected({0, 1, 2, 3, 4, 5, 6, 7, 8});

  auto matrix = std::make_shared< MatrixType >();
  auto opGenerator    = std::make_shared< StreamGenerator<InputType> >(stream, sizeMatrix);
  auto opToMatrix     = std::make_shared< ToMatrix< MatrixType > >(matrix);
  
  CREATE_DATA_LINK(opGenerator, opToMatrix);
  
  opGenerator->start();
        
  REQUIRE(matrix->getRows() == 3);
  REQUIRE(matrix->getCols() == 3);

  matrix->resize(10, 10);

  REQUIRE(matrix->getRows() == 10);
  REQUIRE(matrix->getCols() == 10);

  for(size_t i = 0, rows = 3; i < rows; ++i)
    for(size_t j = 0, cols = 3; j < cols; ++j) {
      auto id = i*rows + j;
      REQUIRE(matrix->get(j, i) == expected[id]);
    }
}

TEST_CASE("Remove values from Dense matrix", "[MatrixOperations]")
{
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  const int sizeMatrix = 9;
  const std::vector<CellType>     expected({0, 1, 0, 3, 4, 0, 6, 7, 0});
  const std::vector<InputType>    removedVal = {makeTuplePtr(2, 0, 0), makeTuplePtr(2, 1, 0), makeTuplePtr(2, 2, 0) };

  auto matrix         = std::make_shared< MatrixType >();
  auto opGenerator    = std::make_shared< StreamGenerator<InputType> >(stream, sizeMatrix);
  auto opToMatrix     = std::make_shared< ToMatrix< MatrixType > >(matrix);
  
  CREATE_DATA_LINK(opGenerator, opToMatrix);
  
  opGenerator->start();
  
  REQUIRE(matrix->getRows() == 3);
  REQUIRE(matrix->getCols() == 3);

  for(auto &tuple : removedVal) {
    matrix->erase(tuple);
  }

  for(size_t i = 0, rows = matrix->getRows(); i < rows; ++i)
    for(size_t j = 0, cols = matrix->getCols(); j < cols; ++j) {
      auto id = i*rows + j;
      REQUIRE(matrix->get(j, i) == expected[id]);
    }  
}

TEST_CASE("Remove values from Sparse matrix", "[MatrixOperations]")
{
  typedef Matrix<CellType, ReaderValue<InputType> > MatrixType;

  const std::size_t size = 50;
  std::vector<InputType> inputs; inputs.reserve(size);

  for(auto i = 0u; i < size; ++i)
  {
    int x = std::rand() % 100 + 0;
    int y = std::rand() % 100 + 0;
    CellType z = std::rand() % 50 + 1;
    inputs.push_back(makeTuplePtr(x, y, z));
  }
  std::vector<InputType> inputs2;
  auto matrix = make_shared<MatrixType>();

  auto mockup = std::make_shared<StreamMockup<InputType, InputType> >(inputs, inputs2);  
  
  auto op = std::make_shared< ToMatrix< MatrixType > > (matrix);

  CREATE_DATA_LINK(mockup, op);

  mockup->start();

  REQUIRE(matrix->getNumElements() == size);

  for (auto i = 0u; i < 10; ++i) {
    auto& tuple = inputs[i];
    matrix->erase(tuple);
  }

  REQUIRE(matrix->getNumElements() == size-10);

  for(std::size_t i = 10; i < size; ++i)
  {
    auto tuple = inputs[i];

    auto x = get<0>(tuple);
    auto y = get<1>(tuple);
    auto z = get<2>(tuple);

    REQUIRE(matrix->get(x, y) == z);
  }
}

TEST_CASE("Compare two dense matrices", "[MatrixOperations] [DenseMatrixStream]")
{
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  const int sizeMatrix = 9;

  auto matrix1 = std::make_shared< MatrixType >();
  auto matrix2 = std::make_shared< MatrixType >();

  Topology t;
 
  auto s = t.streamFromGenerator<InputType>(stream, sizeMatrix)
  .toMatrix< MatrixType >(matrix1)
  .toMatrix< MatrixType >(matrix2)
  ;

  t.start(false);

  REQUIRE(*matrix1.get() == *matrix2.get());
}

TEST_CASE("Compare two sparse matrices", "[MatrixOperations] [SparseMatrix]")
{
  typedef Matrix<CellType, ReaderValue<InputType> > MatrixType;

  MatrixType m1;
  MatrixType m2;

  for(auto i = 0u; i < 50; ++i)
  {
    int x = std::rand() % 100 + 0;
    int y = std::rand() % 100 + 0;
    CellType z = std::rand() % 50 + 0;
    m1.set(x, y, z);
    m2.set(x, y, z);
    REQUIRE(m1.get(x, y) == z);
    REQUIRE(m2.get(x, y) == z);
    REQUIRE(m1.get(x, y) == m2.get(x, y));
  }

  REQUIRE(m1.getRows() == m2.getRows());
  REQUIRE(m1.getCols() == m2.getCols());
  REQUIRE(m1 == m2); 
}

TEST_CASE("Remove row for dense matrix", "[MatrixOperations]")
{
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  const int sizeMatrix = 9;
  const std::vector<CellType>     expected({0, 1, 3, 4, 6, 7});

  auto matrix = std::make_shared< MatrixType >();
  auto opGenerator    = std::make_shared< StreamGenerator<InputType> >(stream, sizeMatrix);
  auto opToMatrix     = std::make_shared< ToMatrix< MatrixType > >(matrix);
  
  CREATE_DATA_LINK(opGenerator, opToMatrix);
  
  opGenerator->start(); 

  matrix->removeRow(2);

  REQUIRE(matrix->getRows() == 2);
  REQUIRE(matrix->getCols() == 3);

  for(size_t i = 0, cols = matrix->getCols(); i < cols; ++i)
    for(size_t j = 0, rows = matrix->getRows(); j < rows; ++j) {
      auto id = i*rows + j;
      REQUIRE(matrix->get(j, i) == expected[id]);
    }
}

TEST_CASE("Remove col for dense matrix", "[MatrixOperations]")
{
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  const int sizeMatrix = 9;
  const std::vector<CellType>     expected({3, 4, 5, 6, 7, 8});

  auto matrix = std::make_shared< MatrixType >();
  auto opGenerator    = std::make_shared< StreamGenerator<InputType> >(stream, sizeMatrix);
  auto opToMatrix     = std::make_shared< ToMatrix< MatrixType > >(matrix);
  
  CREATE_DATA_LINK(opGenerator, opToMatrix);
  
  opGenerator->start(); 


  matrix->removeCol(0);

  for(size_t i = 0, cols = matrix->getCols(); i < cols; ++i)
    for(size_t j = 0, rows = matrix->getRows(); j < rows; ++j) {
      auto id = i*rows + j;
      REQUIRE(matrix->get(j, i) == expected[id]);
    }
}


TEST_CASE("Insert row vectors and matrix", "[MatrixOperations]")
{
  typedef VectorY<CellType> VectorRow;
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  typedef typename VectorRow::MatrixType EigenVector;
  typedef typename MatrixType::MatrixType EigenMatrix;

  { // Insert row vectors
    const auto rows = 1, cols = 5;
    std::vector<VectorRow> vecs; vecs.resize(5);
    MatrixType matrix;

    for(auto i = 0u; i < vecs.size(); ++i) {
      vecs[i].setMatrix(EigenVector::Random(rows, cols));
    }

    for(auto i = 0u; i < vecs.size(); ++i) {
      matrix.insertRow(i, vecs[i]);
    }

    REQUIRE(matrix.getRows() == (unsigned)vecs.size());
    REQUIRE(matrix.getCols() == cols);

    for(size_t i = 0, rows = matrix.getRows(); i < rows; ++i) 
      for(size_t j = 0, cols = matrix.getCols(); j < cols; ++j)
      {
        REQUIRE(matrix.get(i, j) == vecs[i].get(0, j));
      }
  } 
  { // Insert matrix
    const auto rows = 10, cols = 10, rowId = 2;
    const auto mat_size = 10/2;
    
    MatrixType insertedMat; insertedMat.setMatrix(EigenMatrix::Random(rows, cols));
    EigenMatrix randomMat; randomMat.resize(mat_size, mat_size);
    for(auto i = 0; i < mat_size; ++i)
      for(auto j = 0; j < mat_size; ++j)
        randomMat(i, j) = i*mat_size+j;

    MatrixType matrix; matrix.setMatrix(randomMat);

    matrix.insertRow(rowId, insertedMat);

    REQUIRE(matrix.getRows() == mat_size+rows);
    REQUIRE(matrix.getCols() == cols);

    for(auto i = 0; i < matrix.getRows(); ++i)
      for(auto j = 0; j < matrix.getCols(); ++j) {
          
          if(i < rowId && j < mat_size)
            REQUIRE(matrix.get(i, j) == randomMat(i, j));
          else if(i >= rowId && i < (rowId+rows))
            REQUIRE(matrix.get(i, j) == insertedMat.get(i-rowId, j));
          else if(i >= rowId+rows && j < mat_size) 
            REQUIRE(matrix.get(i, j) == randomMat(i-(rows), j));
    }
  }
}

TEST_CASE("Insert column vectors and matrix", "[MatrixOperations]")
{
  typedef VectorX<CellType> VectorCol;
  typedef DenseMatrixStream<CellType, ReaderValue<InputType> > MatrixType;
  typedef typename VectorCol::MatrixType EigenVector;
  typedef typename MatrixType::MatrixType EigenMatrix;

  {// Insert column vectors
    const auto rows = 5, cols = 1;
    std::vector<VectorCol> vecs; vecs.resize(5);
    MatrixType matrix;

    for(auto i = 0u; i < vecs.size(); ++i) {
      vecs[i].setMatrix(EigenVector::Random(rows, cols));
    }
    for(auto i = 0u; i < vecs.size(); ++i) {
      matrix.insertCol(i, vecs[i]);
      
    }
    REQUIRE(matrix.getRows() == rows);
    REQUIRE(matrix.getCols() == vecs.size());

    for(auto j = 0; j < matrix.getCols(); ++j) 
      for(auto i = 0; i < matrix.getRows(); ++i) {
        REQUIRE(matrix.get(i, j) == vecs[j].get(i, 0));
      }    
  }
  { // Insert matrix
    const auto rows = 10, cols = 10, colId = 2;
    const auto mat_size = rows/2;

    MatrixType insertedMat; insertedMat.setMatrix(EigenMatrix::Random(rows, cols));
    EigenMatrix randomMat; randomMat.resize(mat_size, mat_size);
    for(auto i = 0; i < mat_size; ++i)
      for(auto j = 0; j < mat_size; ++j)
        randomMat(i, j) = i*mat_size+j;

    MatrixType matrix; matrix.setMatrix(randomMat);

    matrix.insertCol(colId, insertedMat);
    REQUIRE(matrix.getRows() == rows);
    REQUIRE(matrix.getCols() == mat_size+cols);

    for(auto j = 0; j < matrix.getCols(); ++j)
      for(auto i = 0; i < matrix.getRows(); ++i) {

        if(j < colId && i < mat_size)
          REQUIRE(matrix.get(i, j) == randomMat(i, j));
        else if (j >= colId && j < colId+cols)
          REQUIRE(matrix.get(i, j) == insertedMat.get(i, j-colId));
        else if (j >= colId+cols && i < mat_size)
          REQUIRE(matrix.get(i, j) == randomMat(i, j-cols));

      }
  }
}
TEST_CASE("Remove row for sparse matrix", "[MatrixOperations]")
{
  typedef Matrix<CellType, ReaderValue<InputType>> MatrixType;
  typedef typename MatrixType::MatrixType SpMat;

  auto sizeMat = 10;
  SpMat srcMat(sizeMat, sizeMat);

  for(auto i = 0; i < srcMat.rows(); ++i)
      for(auto j = 0; j < srcMat.cols(); ++j)
      {
        srcMat.coeffRef(i, j) = i*sizeMat+j;
      }


  MatrixType matrix; matrix.setMatrix(srcMat);

  matrix.removeRow(0);
  matrix.removeRow(2);
  matrix.removeRow(matrix.getRows()-1);

  REQUIRE(matrix.getRows() == 7);
  REQUIRE(matrix.getCols() == 10);

  for(auto j = 0; j < srcMat.cols(); ++j)
    for(auto i = 0; i < srcMat.rows(); ++i)
    {
      if(i >= 1 && i < 3) {
        REQUIRE(matrix.get(i-1, j) == srcMat.coeff(i, j));
      } 
      else if (i >= 4 && i < matrix.getRows()) {
        REQUIRE(matrix.get(i-2, j) == srcMat.coeff(i, j));
      }
    }
}

TEST_CASE("Remove columns from sparse matrix", "[MatrixOperations]")
{
  typedef Matrix<CellType, ReaderValue<InputType>> MatrixType;
  typedef typename MatrixType::MatrixType SpMat;

  auto sizeMat = 10;
  SpMat srcMat(sizeMat, sizeMat);

  for(auto i = 0; i < srcMat.rows(); ++i)
      for(auto j = 0; j < srcMat.cols(); ++j)
      {
        srcMat.coeffRef(i, j) = i*sizeMat+j;
      }


  MatrixType matrix; matrix.setMatrix(srcMat);

  matrix.removeCol(0);
  matrix.removeCol(2);
  matrix.removeCol(matrix.getCols()-1);

  REQUIRE(matrix.getCols() == 7);
  REQUIRE(matrix.getRows() == 10);


  for(auto j = 0; j < srcMat.cols(); ++j)
    for(auto i = 0; i < srcMat.rows(); ++i)
    {
      if(j >= 1 && j < 3) {
        REQUIRE(matrix.get(i, j-1) == srcMat.coeff(i, j));
      }
      else if(j > 3 && j < matrix.getCols()) {
        REQUIRE(matrix.get(i, j-2) == srcMat.coeff(i, j));
      }
     }
}

TEST_CASE("Iterator for dense matrix", "[MatrixOperations] [DenseMatrix]")
{
  typedef DenseMatrix<CellType> MatrixType;
  typedef typename MatrixType::MatrixType EigenMatrix;

  auto size = 10;
  auto id = 0;

  EigenMatrix srcMat(size, size);
  for(auto i = 0; i < size; ++i)
    for(auto j = 0; j < size; ++j, ++id)
      srcMat(i, j) = id;

  MatrixType matrix; matrix.setMatrix(srcMat); 

  auto it = matrix.begin();

  for(auto beg = matrix.begin(), end = matrix.end(); beg != end; ++beg){
    auto i = beg.getRow(); auto j = beg.getCol();
    
    REQUIRE(srcMat(i, j) == *beg);
  }
}

TEST_CASE("Insert at the end of the matrix/vector", "[MatrixOperations][DenseMatrix]")
{
  typedef DenseMatrix<CellType> MatrixType;
  typedef typename MatrixType::MatrixType DMatrix;
  
  {
    auto size = 50;
    DMatrix srcMat = DMatrix::Random(size, 1);
    MatrixType matrix;
  
    for(auto i = 0; i < size; ++i) {
      matrix.add2end(srcMat(i, 0));
    }
  
    for(auto i = 0; i < size; ++i) {
      REQUIRE(matrix(i, 0) == srcMat(i, 0));
    }
  }
  {
    auto size = 50;
    DMatrix srcMat(DMatrix::Random(size, size));
    MatrixType matrix(srcMat);
    auto vecSize = 5;
    DMatrix vector = DMatrix::Random(vecSize, 1);

    REQUIRE(matrix.getMatrix() == srcMat);

    for(auto i = 0; i < vecSize; ++i) {
      matrix.add2end(vector(i, 0));
    }

    REQUIRE(matrix.getRows() == size+vecSize);
    REQUIRE(matrix.getCols() == size);
    for(auto i = size, j = size-1, vecID = 0; i < size+vecSize; ++i, ++vecID) {
      REQUIRE(matrix(i, j) == vector(vecID, 0));
    }
  }
}

TEST_CASE("Reconstruct the matrix by incident indexes", "[MatrixOperations][DenseMatrix]")
{
  typedef DenseMatrix<CellType> MatrixType;
  typedef typename MatrixType::MatrixType DMatrix;
  typedef typename MatrixType::Triplet Triplet;

  auto size = 50, incidentSize = 25;
  DMatrix dMat(DMatrix::Random(size, size));
  MatrixType matrix;
  std::vector<Triplet> triplets(incidentSize);

  for(auto i = 0; i < incidentSize; ++i)
  {
    auto row = std::rand()%(size-2) + 0;
    auto col = std::rand()%(size-2) + 0;
    auto value = dMat(row, col);

    matrix.addIncident(row, col, value);
    triplets[i] = std::make_tuple(row, col, value);
  }
  REQUIRE(matrix.getCountIncidents() == incidentSize);
  REQUIRE(matrix.getRows() == incidentSize);
  REQUIRE(matrix.getCols() == 1);

  for(auto i = 0u; i < matrix.getCountIncidents(); ++i) {
    auto srcTriple = triplets[i];
    auto triple = matrix.getIncident(i);

    REQUIRE(srcTriple == triple);
  }
}