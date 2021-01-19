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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include "pfabric.hpp"
#include "matrix/DenseMatrix.hpp"
#include "operations/filters/GaussianFilter.hpp"

#include <fstream>
#include <sstream>

using namespace pfabric;

TEST_CASE("Gaussian blur image filter", "[ImageProcessing]")
{
  typedef float                         CellType;
  typedef VectorY<CellType>             VectorCol;
  typedef TuplePtr<int, int, VectorCol >  InputType;
  typedef TuplePtr<VectorCol> VectorTuple;
  GaussianFilter filter(5, 3, CV_32FC3);
  
  pfabric::Topology t;

  std::ostringstream img_values;
  t.newStreamFromFile(std::string(TEST_DATA_DIRECTORY)+"blur_image_test.in")
   .extract<InputType>(',')
   .map<VectorTuple>([](const auto& tp, bool) {
      return makeTuplePtr(pfabric::get<2>(tp));
   })
   .map<VectorTuple>([&filter](const auto& tp, bool) {
      auto &vector = get<0>(tp);
      filter.apply(vector.getRawData(), vector.getRows(), vector.getCols());
      return tp;
    })
   .print(img_values)
   ;

  t.start(false);

  std::ifstream f(std::string(TEST_DATA_DIRECTORY)+"blur_image_test.res");
  std::stringstream expected;
  if(f.is_open()) {
  
    expected << f.rdbuf();

    REQUIRE(img_values.str() == expected.str());
  } else {
    REQUIRE(false);
  }
}
