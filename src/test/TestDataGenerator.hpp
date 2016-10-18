#ifndef TestDataGenerator_hpp_
#define TestDataGenerator_hpp_

#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#ifdef COMPRESSED_FILE_SOURCE
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#endif
#include <boost/iostreams/copy.hpp>
#include <boost/algorithm/string.hpp>
 #include <boost/iostreams/stream.hpp>
#include "fmt/format.h"

namespace pfabric {

class TestDataGenerator {
public:
  TestDataGenerator(const std::string& fname) : fileName(fname), isCompressed(false) {
  }

  ~TestDataGenerator() { cleanup(); }

  void writeData(int ntuples, bool compressed = false) {
    std::ofstream ofs(fileName);

    for (int i = 0; i < ntuples; i++) {
        ofs << fmt::format("{},This is a string field,{}\n", i, i * 100 + 0.5);
        // i << "," << "This is a string field" << "," << i * 100 + 0.5 << '\n';
    }
    ofs.close();
#ifdef COMPRESSED_FILE_SOURCE
    if (compressed) {
      namespace io = boost::iostreams;

      isCompressed = true;
      io::file_source infile(fileName);
      io::filtering_istream fis;
      io::gzip_compressor gzip;
      fis.push(gzip);
      fis.push(infile);

      io::file_sink outfile(fileName + std::string(".gz"));
      io::stream<io::file_sink> os(outfile);
      io::copy(fis, os);
    }
#endif
  }

  void cleanup() {
    boost::filesystem::remove(fileName);
#ifdef COMPRESSED_FILE_SOURCE
    if (isCompressed)
      boost::filesystem::remove(fileName + std::string(".gz"));
#endif
  }

  private:
  std::string fileName;
  bool isCompressed;
};

}

#endif
