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

#include <iostream>

#include "pfabric.hpp"

using namespace pfabric;

// the structure of tuples we receive via REST
// typedef TuplePtr<Tuple<int, double> > InTuplePtr;
typedef TuplePtr<int, double> InTuplePtr;

// the structure of aggregate tuples
// typedef TuplePtr<Tuple<double> > ResultTuplePtr;

// the aggregate operator needs a state object that is defined here:
// template parameters are: the input type,
//                          the aggregate function (Avg on double),
//                          the column of the input tuple on which we calculate the aggregate
typedef Aggregator1<InTuplePtr, AggrAvg<double, double>, 1> MyAggrState;

int main(int argc, char **argv) {
  PFabricContext ctx;

  auto t = ctx.createTopology();

  auto s = t->newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
    .extractJson<InTuplePtr>({"key", "data"})
    .slidingWindow(WindowParams::RowWindow, 10)
    .aggregate<MyAggrState>()
    .notify([&](auto tp, bool outdated) { std::cout << tp << std::endl; });
//    .print<ResultTuplePtr>(std::cout);

  t->start();
  std::cout << "running ..." << std::endl;
  t->wait();
}
