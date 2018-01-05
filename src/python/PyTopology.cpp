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

#include <boost/python/enum.hpp>

using namespace pfabric;

namespace bp = boost::python;

PyPipe PyPipe::extract(char sep) {
  auto pipe = boost::get<StringPipe&>(pipeImpl);
  return PyPipe(pipe.map<PyTuplePtr>([sep](auto tp, bool) -> PyTuplePtr {
    bp::list seq;
    auto s = get<0>(tp).begin();
    while (*s) {
      char* item = (char *)s;
      while (*s && *s != sep) s++;
      if ((s - item) == 0) {
        // TODO null
      }
      else {
        std::string str(item, s - item);
        seq.append(str);
      }
      s++;
    }
    return makeTuplePtr(bp::object(bp::tuple(seq)));
  }));
}

PyPipe PyPipe::where(bp::object pred) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.where([pred](auto tp, bool o) {
    return pred(get<0>(tp), o);
  }));
}

PyPipe PyPipe::map(bp::object fun) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.map<PyTuplePtr>([fun](auto tp, bool o) {
    return makeTuplePtr(fun(get<0>(tp), o));
  }));
}

PyPipe PyPipe::slidingWindow(WindowParams::WinType wt, unsigned int size,
  unsigned int interval) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.slidingWindow(wt, size, nullptr, interval));
}

PyPipe PyPipe::notify(bp::object fun) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.notify([fun](auto tp, bool o) {
    fun(get<0>(tp), o);
  }));
}

PyPipe PyPipe::queue() {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.queue());
}

PyPipe PyPipe::assignTimestamps(bp::object fun) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.assignTimestamps([fun](auto tp) -> Timestamp {
    auto res = fun(get<0>(tp));
    return (Timestamp) bp::extract<long>(res);
  }));
}

PyPipe PyPipe::keyBy(bp::object fun) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.keyBy<std::string>([fun](auto tp) -> std::string {
    bp::object res = fun(get<0>(tp));
    const char *s = bp::extract<const char *>(bp::str(res));
    return std::string(s); // bp::extract<std::string>(bp::str(fun(get<0>(tp))));
  }));
}

PyPipe PyPipe::aggregate(bp::list columns, bp::list aggrFuncs) {
  std::vector<int> columnVec;
  std::vector<AggrFuncType> funcVec;
  for (int i = 0; i < bp::len(columns); ++i) {
    columnVec.push_back(bp::extract<int>(columns[i]));
    funcVec.push_back(bp::extract<AggrFuncType>(aggrFuncs[i]));
  }
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  auto state = std::make_shared<PyAggregateState>(columnVec, funcVec);
  return PyPipe(pipe.aggregate<PyTuplePtr, PyAggregateState>(state, PyAggregateState::finalize,
    PyAggregateState::iterate));
}

PyPipe PyPipe::groupBy(bp::list columns, bp::list aggrFuncs) {
  std::vector<int> columnVec;
  std::vector<AggrFuncType> funcVec;

  columnVec.push_back(0);
  funcVec.push_back(AggrFuncType::GroupID);

  for (int i = 0; i < bp::len(columns); ++i) {
    columnVec.push_back(bp::extract<int>(columns[i]));
    funcVec.push_back(bp::extract<AggrFuncType>(aggrFuncs[i]));
  }
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  auto state = std::make_shared<PyAggregateState>(columnVec, funcVec);
  return PyPipe(pipe.groupBy<PyTuplePtr, PyAggregateState, std::string>(state,
    PyAggregateState::create,
    PyAggregateState::finalize, PyAggregateState::iterateForKey));
}

PyPipe PyPipe::print() {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  auto pyFormatter = [](std::ostream& os, auto tp) {
    auto pyObj = get<0>(tp);
    for (auto i = 0; i < len(pyObj); i++) {
      if (i > 0) os << ", ";
      os << bp::extract<std::string>(bp::str(pyObj[i]))();
    }
    os << std::endl;
  };
  return PyPipe(pipe.print(std::cout, pyFormatter));
}

PyPipe PyPipe::join(PyPipe other, bp::object pred) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  auto otherPipe = boost::get<TuplePipe&>(other.pipeImpl);

  auto joinPipe = pipe.join<std::string>(otherPipe, [pred](auto tp1, auto tp2) {
    return pred(get<0>(tp1), get<0>(tp2));
  });

  return PyPipe(joinPipe.map<PyTuplePtr>([](auto tp, bool o) -> PyTuplePtr {
    bp::list seq;
    auto tp1 = get<0>(tp);
    for (auto i = 0; i < bp::len(tp1); i++) seq.append(tp1[i]);
    auto tp2 = get<1>(tp);
    for (auto i = 0; i < bp::len(tp2); i++) seq.append(tp2[i]);

    return makeTuplePtr(bp::object(bp::tuple(seq)));
  }));
}

PyTopology::PyTopology() {
  topo = ctx.createTopology();
}

PyPipe PyTopology::newStreamFromFile(std::string file) {
  return PyPipe(topo->newStreamFromFile(file));
}

void PyTopology::start() {
    topo->start(false);
}

BOOST_PYTHON_MODULE(pyfabric) {
    bp::class_<pfabric::PyTopology>("Topology")
    .def("stream_from_file", &pfabric::PyTopology::newStreamFromFile)
      .def("start", &pfabric::PyTopology::start)
    ;

    bp::class_<pfabric::PyPipe>("Pipe", bp::no_init)
      .def("extract", &pfabric::PyPipe::extract)
        .def("where", &pfabric::PyPipe::where)
        .def("map", &pfabric::PyPipe::map)
        .def("assign_timestamps", &pfabric::PyPipe::assignTimestamps)
        .def("key_by", &pfabric::PyPipe::keyBy)
        .def("aggregate", &pfabric::PyPipe::aggregate)
        .def("groupby_key", &pfabric::PyPipe::groupBy)
        .def("sliding_window", &pfabric::PyPipe::slidingWindow)
        .def("join", &pfabric::PyPipe::join)
        .def("queue", &pfabric::PyPipe::queue)
        .def("notify", &pfabric::PyPipe::notify)
        .def("pfprint", &pfabric::PyPipe::print)
        .def("notify", &pfabric::PyPipe::notify)
    ;

    bp::enum_<pfabric::AggrFuncType>("aggr")
        .value("Int", pfabric::AggrFuncType::IntIdentity)
        .value("String", pfabric::AggrFuncType::StringIdentity)
        .value("Double", pfabric::AggrFuncType::DoubleIdentity)
        .value("IntSum", pfabric::AggrFuncType::IntSum)
        .value("DoubleSum", pfabric::AggrFuncType::DoubleSum)
        .value("IntAvg", pfabric::AggrFuncType::IntAvg)
        .value("DoubleAvg", pfabric::AggrFuncType::DoubleAvg)
        .value("Count", pfabric::AggrFuncType::Count)
        .value("DistinctCount", pfabric::AggrFuncType::DCount)
        .value("IntMin", pfabric::AggrFuncType::IntMin)
        .value("DoubleMin", pfabric::AggrFuncType::DoubleMin)
        .value("StringMin", pfabric::AggrFuncType::StringMin)
        .value("IntMax", pfabric::AggrFuncType::IntMax)
        .value("DoubleMax", pfabric::AggrFuncType::DoubleMax)
        .value("StringMax", pfabric::AggrFuncType::StringMax)
        ;

    bp::enum_<pfabric::WindowParams::WinType>("wintype")
        .value("range", pfabric::WindowParams::RangeWindow)
        .value("row", pfabric::WindowParams::RowWindow)
        ;

}
