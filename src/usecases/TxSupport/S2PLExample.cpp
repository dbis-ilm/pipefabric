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

/**
 * This demo illustrates transactional data stream processing. One topology
 * produces a stream of elements which consists of individual transactions
 * marked by BEGIN and COMMIT. The stream elements are used to update a
 * relational table. A second batch topology (query) reads this table
 * periodically. The transactional context guarantees snapshot isolation
 * of this query.
 */

#include "TxExample.hpp"

using namespace pfabric;

using TableType = S2PLTable<AccountPtr::element_type, uint_t>;

constexpr auto protocol = "S2PL";
constexpr auto scaling = false;

int main() {
  ofstream resFileRec;
  resFileRec.open(resultFileRec, ios::out | ios::app);
  auto start = std::chrono::high_resolution_clock::now();
  TxExample<TableType> s2pl{protocol, scaling};
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  resFileRec << protocol << ',' << keyRange << ',' << simReaders << ",Context Recovery," << diff
    << '\n';
  resFileRec.close();
  s2pl.run();
}
