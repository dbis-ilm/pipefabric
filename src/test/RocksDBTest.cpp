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

#include "catch.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "rocksdb/db.h"

#include "core/Tuple.hpp"
#include "core/serialize.hpp"
#include "pfabric.hpp"

#include "fmt/format.h"

using namespace std;

template <class T>
inline rocksdb::Slice valToSlice(const T& t) {
  return rocksdb::Slice(reinterpret_cast<const char*>(&t), sizeof(t));
}

template <class T>
inline T& sliceToVal(const rocksdb::Slice& slice) {
  return *(reinterpret_cast<T*>(const_cast<char*>(slice.data())));
}

template <class T>
inline T sliceToTuple(const rocksdb::Slice& slice) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(slice.data());
  StreamType buf(ptr, ptr + slice.size());
  return T(buf);
}

typedef pfabric::Tuple<unsigned long, int, string, double> MyTuple;
typedef pfabric::TuplePtr<MyTuple> MyTuplePtr;

TEST_CASE("Testing storing tuples in RocksDB", "[RocksDB]") {
  // Set up database connection information and open database
  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::Status status = rocksdb::DB::Open(options, "./testdb", &db);

  REQUIRE(status.ok());

      // Add 256 values to the database
      rocksdb::WriteOptions writeOptions;
  for (unsigned int i = 0; i < 256; i++) {
    auto tup = MyTuple((unsigned long)i + 1, (i + 1) * 100,
                       fmt::format("String #{0}", i), i * 12.345);
    StreamType buf;
    tup.serializeToStream(buf);

    status = db->Put(
        writeOptions, valToSlice(i),
        rocksdb::Slice(reinterpret_cast<const char*>(buf.data()), buf.size()));
    REQUIRE(status.ok());
  }

  // Iterate over each item in the database and print them
  rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());

  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    int k = sliceToVal<int>(it->key());
    auto tup = sliceToTuple<MyTuple>(it->value());
    auto expected = MyTuple((unsigned long)k + 1, (k + 1) * 100,
                       fmt::format("String #{0}", k), k * 12.345);
    REQUIRE(tup == expected);
    cout << sliceToVal<int>(it->key()) << " : " << tup << endl;
  }

  REQUIRE(it->status().ok());

  delete it;

  // Close the database
  delete db;
  rocksdb::DestroyDB("./testdb", rocksdb::Options());
}
