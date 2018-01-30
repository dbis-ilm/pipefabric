/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

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
