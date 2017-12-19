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

PyPipe PyPipe::notify(bp::object fun) {
  auto pipe = boost::get<TuplePipe&>(pipeImpl);
  return PyPipe(pipe.notify([fun](auto tp, bool o) {
    return fun(get<0>(tp), o);
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

BOOST_PYTHON_MODULE(libpfabric) {
    bp::class_<pfabric::PyTopology>("Topology")
    .def("newStreamFromFile", &pfabric::PyTopology::newStreamFromFile)
      .def("start", &pfabric::PyTopology::start)
    ;

    bp::class_<pfabric::PyPipe>("Pipe", bp::no_init)
      .def("extract", &pfabric::PyPipe::extract)
        .def("where", &pfabric::PyPipe::where)
        .def("map", &pfabric::PyPipe::map)
        .def("pfprint", &pfabric::PyPipe::print)
        .def("notify", &pfabric::PyPipe::notify)
    ;
}
