/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */
#include "PyTopology.hpp"

using namespace pfabric;

namespace bp = boost::python;

PyAggregateState::PyAggregateState(const std::vector<int>& cols,
  const std::vector<AggrFuncType>& funcs) : mColumns(cols), mFuncSpecs(funcs) {
  for (auto f : mFuncSpecs) {
    switch (f) {
      case AggrFuncType::IntSum:
        mAggrFuncs.push_back(new AggrSum<int>());
        break;
      case AggrFuncType::Count:
        mAggrFuncs.push_back(new AggrCount<int, int>());
        break;
    }
  }
}

void PyAggregateState::init() {
  for (auto& func : mAggrFuncs) {
    func->init();
  }
}

void PyAggregateState::iterate(const PyTuplePtr& tp,
  AggrStatePtr state, const bool outdated) {
  auto tup = get<0>(tp);
  for (std::size_t i = 0; i < state->mFuncSpecs.size(); i++) {
    switch (state->mFuncSpecs[i]) {
      case AggrFuncType::IntSum: {
        AggrSum<int> *aggr = dynamic_cast<AggrSum<int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(tup[i]);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::Count: {
        AggrCount<int, int> *aggr = dynamic_cast<AggrCount<int, int>*>(state->mAggrFuncs[i]);
        aggr->iterate(1, outdated);
        break;
      }
    }
  }
}

PyTuplePtr PyAggregateState::finalize(AggrStatePtr state) {
  bp::list seq;
  for (std::size_t i = 0; i < state->mFuncSpecs.size(); i++) {
    switch (state->mFuncSpecs[i]) {
      case AggrFuncType::IntSum: {
        AggrSum<int> *aggr = dynamic_cast<AggrSum<int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::Count: {
        AggrCount<int, int> *aggr = dynamic_cast<AggrCount<int, int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
    }
  }
  return makeTuplePtr(bp::object(bp::tuple(seq)));
}
