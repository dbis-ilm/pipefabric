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
    setupAggregateFuncs();
}

PyAggregateState::PyAggregateState(const PyAggregateState& s) :
  mColumns(s.mColumns), mFuncSpecs(s.mFuncSpecs)  {
  setupAggregateFuncs();
}

void PyAggregateState::setupAggregateFuncs() {
  for (auto f : mFuncSpecs) {
    switch (f) {
      case AggrFuncType::GroupID:
        mAggrFuncs.push_back(new AggrIdentity<std::string>());
        break;
      case AggrFuncType::IntIdentity:
        mAggrFuncs.push_back(new AggrIdentity<int>());
        break;
      case AggrFuncType::DoubleIdentity:
        mAggrFuncs.push_back(new AggrIdentity<double>());
        break;
      case AggrFuncType::StringIdentity:
        mAggrFuncs.push_back(new AggrIdentity<std::string>());
        break;
      case AggrFuncType::IntSum:
        mAggrFuncs.push_back(new AggrSum<int>());
        break;
      case AggrFuncType::DoubleSum:
        mAggrFuncs.push_back(new AggrSum<double>());
        break;
      case AggrFuncType::Count:
        mAggrFuncs.push_back(new AggrCount<int, int>());
        break;
      case AggrFuncType::DCount:
        mAggrFuncs.push_back(new AggrDCount<int, int>());
        break;
      case AggrFuncType::IntAvg:
        mAggrFuncs.push_back(new AggrAvg<int, int>());
        break;
      case AggrFuncType::DoubleAvg:
        mAggrFuncs.push_back(new AggrAvg<double, double>());
        break;
      case AggrFuncType::IntMin:
        mAggrFuncs.push_back(new AggrMinMax<int, std::less<int>>());
        break;
      case AggrFuncType::DoubleMin:
        mAggrFuncs.push_back(new AggrMinMax<double, std::less<double>>());
        break;
      case AggrFuncType::StringMin:
        mAggrFuncs.push_back(new AggrMinMax<std::string, std::less<std::string>>());
        break;
      case AggrFuncType::IntMax:
        mAggrFuncs.push_back(new AggrMinMax<int, std::greater<int>>());
        break;
      case AggrFuncType::DoubleMax:
        mAggrFuncs.push_back(new AggrMinMax<double, std::greater<double>>());
        break;
      case AggrFuncType::StringMax:
        mAggrFuncs.push_back(new AggrMinMax<std::string, std::greater<std::string>>());
        break;
    }
  }
}

void PyAggregateState::init() {
  for (auto& func : mAggrFuncs) {
    func->init();
  }
}

void PyAggregateState::iterateForKey(const PyTuplePtr& tp, const std::string& key,
  AggrStatePtr state, const bool outdated) {
  auto tup = get<0>(tp);
  for (std::size_t i = 0; i < state->mFuncSpecs.size(); i++) {
    auto pyObj = tup[state->mColumns[i]];
    switch (state->mFuncSpecs[i]) {
      case AggrFuncType::GroupID: {
        AggrIdentity<std::string> *aggr = dynamic_cast<AggrIdentity<std::string>*>(state->mAggrFuncs[i]);
        aggr->iterate(key, outdated);
        break;
      }
      case AggrFuncType::IntIdentity: {
        AggrIdentity<int> *aggr = dynamic_cast<AggrIdentity<int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleIdentity: {
        AggrIdentity<double> *aggr = dynamic_cast<AggrIdentity<double>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::StringIdentity: {
        AggrIdentity<std::string> *aggr = dynamic_cast<AggrIdentity<std::string>*>(state->mAggrFuncs[i]);
        std::string val = bp::extract<std::string>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntSum: {
        AggrSum<int> *aggr = dynamic_cast<AggrSum<int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleSum: {
        AggrSum<double> *aggr = dynamic_cast<AggrSum<double>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::Count: {
        AggrCount<int, int> *aggr = dynamic_cast<AggrCount<int, int>*>(state->mAggrFuncs[i]);
        aggr->iterate(1, outdated);
        break;
      }
      case AggrFuncType::DCount: {
        AggrDCount<int, int> *aggr = dynamic_cast<AggrDCount<int, int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntAvg: {
        AggrAvg<int, int> *aggr = dynamic_cast<AggrAvg<int, int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleAvg: {
        AggrAvg<double, double> *aggr = dynamic_cast<AggrAvg<double, double>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntMin: {
        AggrMinMax<int, std::less<int>> *aggr =
          dynamic_cast<AggrMinMax<int, std::less<int>>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleMin: {
        AggrMinMax<double, std::less<double>> *aggr =
          dynamic_cast<AggrMinMax<double, std::less<double>>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::StringMin: {
        AggrMinMax<std::string, std::less<std::string>> *aggr =
          dynamic_cast<AggrMinMax<std::string, std::less<std::string>>*>(state->mAggrFuncs[i]);
        std::string val = bp::extract<std::string>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntMax: {
        AggrMinMax<int, std::greater<int>> *aggr =
          dynamic_cast<AggrMinMax<int, std::greater<int>>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleMax: {
        AggrMinMax<double, std::greater<double>> *aggr =
          dynamic_cast<AggrMinMax<double, std::greater<double>>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::StringMax: {
        AggrMinMax<std::string, std::greater<std::string>> *aggr =
          dynamic_cast<AggrMinMax<std::string, std::greater<std::string>>*>(state->mAggrFuncs[i]);
        std::string val = bp::extract<std::string>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
    }
  }
}

void PyAggregateState::iterate(const PyTuplePtr& tp,
  AggrStatePtr state, const bool outdated) {
  auto tup = get<0>(tp);
  for (std::size_t i = 0; i < state->mFuncSpecs.size(); i++) {
    auto pyObj = tup[state->mColumns[i]];
    switch (state->mFuncSpecs[i]) {
      case AggrFuncType::IntIdentity: {
        AggrIdentity<int> *aggr = dynamic_cast<AggrIdentity<int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleIdentity: {
        AggrIdentity<double> *aggr = dynamic_cast<AggrIdentity<double>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::StringIdentity: {
        AggrIdentity<std::string> *aggr = dynamic_cast<AggrIdentity<std::string>*>(state->mAggrFuncs[i]);
        std::string val = bp::extract<std::string>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntSum: {
        AggrSum<int> *aggr = dynamic_cast<AggrSum<int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleSum: {
        AggrSum<double> *aggr = dynamic_cast<AggrSum<double>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::Count: {
        AggrCount<int, int> *aggr = dynamic_cast<AggrCount<int, int>*>(state->mAggrFuncs[i]);
        aggr->iterate(1, outdated);
        break;
      }
      case AggrFuncType::DCount: {
        AggrDCount<int, int> *aggr = dynamic_cast<AggrDCount<int, int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntAvg: {
        AggrAvg<int, int> *aggr = dynamic_cast<AggrAvg<int, int>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleAvg: {
        AggrAvg<double, double> *aggr = dynamic_cast<AggrAvg<double, double>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntMin: {
        AggrMinMax<int, std::less<int>> *aggr =
          dynamic_cast<AggrMinMax<int, std::less<int>>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleMin: {
        AggrMinMax<double, std::less<double>> *aggr =
          dynamic_cast<AggrMinMax<double, std::less<double>>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::StringMin: {
        AggrMinMax<std::string, std::less<std::string>> *aggr =
          dynamic_cast<AggrMinMax<std::string, std::less<std::string>>*>(state->mAggrFuncs[i]);
        std::string val = bp::extract<std::string>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::IntMax: {
        AggrMinMax<int, std::greater<int>> *aggr =
          dynamic_cast<AggrMinMax<int, std::greater<int>>*>(state->mAggrFuncs[i]);
        int val = bp::extract<int>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::DoubleMax: {
        AggrMinMax<double, std::greater<double>> *aggr =
          dynamic_cast<AggrMinMax<double, std::greater<double>>*>(state->mAggrFuncs[i]);
        double val = bp::extract<double>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
      case AggrFuncType::StringMax: {
        AggrMinMax<std::string, std::greater<std::string>> *aggr =
          dynamic_cast<AggrMinMax<std::string, std::greater<std::string>>*>(state->mAggrFuncs[i]);
        std::string val = bp::extract<std::string>(pyObj);
        aggr->iterate(val, outdated);
        break;
      }
    }
  }
}

PyTuplePtr PyAggregateState::finalize(AggrStatePtr state) {
  bp::list seq;
  for (std::size_t i = 0; i < state->mFuncSpecs.size(); i++) {
    switch (state->mFuncSpecs[i]) {
      case AggrFuncType::GroupID: {
        AggrIdentity<std::string> *aggr = dynamic_cast<AggrIdentity<std::string>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::IntIdentity: {
        AggrIdentity<int> *aggr = dynamic_cast<AggrIdentity<int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::DoubleIdentity: {
        AggrIdentity<double> *aggr = dynamic_cast<AggrIdentity<double>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::StringIdentity: {
        AggrIdentity<std::string> *aggr = dynamic_cast<AggrIdentity<std::string>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::IntSum: {
        AggrSum<int> *aggr = dynamic_cast<AggrSum<int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::DoubleSum: {
        AggrSum<double> *aggr = dynamic_cast<AggrSum<double>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::Count: {
        AggrCount<int, int> *aggr = dynamic_cast<AggrCount<int, int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::DCount: {
        AggrDCount<int, int> *aggr = dynamic_cast<AggrDCount<int, int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::IntAvg: {
        AggrAvg<int, int> *aggr = dynamic_cast<AggrAvg<int, int>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::DoubleAvg: {
        AggrAvg<double, double> *aggr = dynamic_cast<AggrAvg<double, double>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::IntMin: {
        AggrMinMax<int, std::less<int>> *aggr =
          dynamic_cast<AggrMinMax<int, std::less<int>>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::DoubleMin: {
        AggrMinMax<double, std::less<double>> *aggr =
          dynamic_cast<AggrMinMax<double, std::less<double>>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::StringMin: {
        AggrMinMax<std::string, std::less<std::string>> *aggr =
          dynamic_cast<AggrMinMax<std::string, std::less<std::string>>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::IntMax: {
        AggrMinMax<int, std::greater<int>> *aggr =
          dynamic_cast<AggrMinMax<int, std::greater<int>>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::DoubleMax: {
        AggrMinMax<double, std::greater<double>> *aggr =
          dynamic_cast<AggrMinMax<double, std::greater<double>>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
      case AggrFuncType::StringMax: {
        AggrMinMax<std::string, std::greater<std::string>> *aggr =
          dynamic_cast<AggrMinMax<std::string, std::greater<std::string>>*>(state->mAggrFuncs[i]);
        seq.append(aggr->value());
        break;
      }
    }
  }
  return makeTuplePtr(bp::object(bp::tuple(seq)));
}
