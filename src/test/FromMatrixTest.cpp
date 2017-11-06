#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "pfabric.hpp"
#include "matrix/Matrix.hpp"
#include "matrix/ReaderValue.hpp"
#include "qop/FromMatrix.hpp"
#include "StreamMockup.hpp"

#include <vector>

using namespace pfabric;

typedef double CellType;
typedef TuplePtr<int, int, CellType> InputType;

TEST_CASE("Stream from matrix", "[FromMatrixTest]")
{
	typedef Matrix<CellType, ReaderValue<InputType> > MatrixType;

  	const std::size_t size = 50;
  	std::vector<InputType> inputs; inputs.reserve(size);
    std::vector<InputType>  inputs2;
  	for(auto i = 0u; i < size; ++i)
  	{
  	  int x = std::rand() % 100 + 0;
  	  int y = std::rand() % 100 + 0;
  	  CellType z = std::rand() % 50 + 0;
  	  inputs.push_back(makeTuplePtr(x, y, z));
  	}
  	auto matrix 		     = std::make_shared< MatrixType	>();
  	auto opStreamMatrix  = std::make_shared< FromMatrix<MatrixType>	>(matrix);
  	auto mockup 		     = std::make_shared< StreamMockup<InputType, InputType> >(inputs, inputs); 

  	CREATE_DATA_LINK(opStreamMatrix, mockup);

    for(auto &tuple : inputs) {
      matrix->insert(tuple);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);

  	REQUIRE(mockup->numTuplesProcessed() == size);
}