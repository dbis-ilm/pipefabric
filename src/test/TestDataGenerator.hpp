/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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
  TestDataGenerator(const std::string& fname) :
  fileName(fname)
#ifdef COMPRESSED_FILE_SOURCE
  , isCompressed(false)
#endif
  {
  }

  ~TestDataGenerator() { cleanup(); }

  void writeData(int ntuples, bool compressed = false) {
    std::ofstream ofs(fileName);

    for (int i = 0; i < ntuples; i++) {
        ofs << fmt::format("{},This is a string field,{:.1f}\n", i, (double)(i * 100 + 0.5));
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
#ifdef COMPRESSED_FILE_SOURCE
  bool isCompressed;
#endif  
};

}

#endif
