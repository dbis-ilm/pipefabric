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