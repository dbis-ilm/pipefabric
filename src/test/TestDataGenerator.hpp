#ifndef TestDataGenerator_hpp_
#define TestDataGenerator_hpp_

#include <string>
#include <fstream>

#include <boost/filesystem.hpp>

#include "fmt/format.h"

namespace pfabric {

class TestDataGenerator {
public:
  TestDataGenerator(const std::string& fname) : fileName(fname) {}

  ~TestDataGenerator() { cleanup(); }

  void writeData(int ntuples) {
    std::ofstream ofs (fileName);
    for (int i = 0; i < ntuples; i++) {
        ofs << fmt::format("{},This is a string field,{}\n", i, i * 100 + 0.5);
        // i << "," << "This is a string field" << "," << i * 100 + 0.5 << '\n';
    }
    ofs.close();
  }

  void cleanup() { boost::filesystem::remove(fileName); }

  private:
  std::string fileName;
};

}

#endif
