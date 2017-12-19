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
#ifndef PyTopology_hpp_
#define PyTopology_hpp_

#include "pfabric.hpp"
#include <boost/python.hpp>
#include <boost/variant.hpp>

namespace bp = boost::python;

namespace pfabric {

/// We handle only tuples consisting of a single field that represents
/// a Python tuple.
typedef TuplePtr<bp::object> PyTuplePtr;

/**
 * @brief PyPipe represents a sequence of operators applied to a data stream.
 *
 * PyPipe objects are used to construct a dataflow programatically in Python.
 * Actually, PyPipe is just a wrapper around the @c Pipe class.
 */
struct PyPipe {
  typedef Pipe<TStringPtr> StringPipe;
  typedef Pipe<PyTuplePtr> TuplePipe;

  ///
  boost::variant<StringPipe, TuplePipe> pipeImpl;

  /**
   * @brief Construct a new PyPipe object from Pipe<TStringPtr>.
   */
  PyPipe(StringPipe p) : pipeImpl(p) {}

  /**
   * @brief Construct a new PyPipe object from Pipe<PyTuplePtr>.
   */
  PyPipe(TuplePipe p) : pipeImpl(p) {}

  /**
   * @brief Creates an data extraction operator.
   *
   * Creates an operator for extracting fields from a simple string
   * tuple as the next operator on the pipe. The result is a Python
   * tuple consisting of string fields.
   *
   * @param[in] sep
   * @return a new PyPipe object
   *
   */
  PyPipe extract(char sep);

  /**
   * @brief Creates a filter operator for selecting tuples.
   *
   * Creates a filter operator which forwards only tuples satisfying the
   * given filter predicate written in Python as the next operator on the pipe.
   *
   * @param[in] pred a function pointer or lambda function implementing a predicate.
   * @return a new PyPipe object
   */
  PyPipe where(bp::object pred);

  /**
   * @brief Creates a projection operator.
   *
   * Creates a map operator which applies a mapping (projection) function
   * written in Python to each tuples as the next operator on the pipe.
   *
   * @param[in] fun  a function pointer or lambda function producing a new tuple
   *                from the input tuple
   * @return a new PyPipe object
   */
  PyPipe map(bp::object fun);

  /**
   * @brief Creates a print operator.
   *
   * Creates an operator for printing tuples to the console
   * as the next operator on the pipe.
   *
   * @return a new PyPipe object
   */
  PyPipe print();

  /**
   * @brief Creates a notification operator.
   *
   * Creates an operator for handling general lambda functions
   * as the next operator on the pipe.
   *
   * @param[in] fun  a function pointer or lambda function applied to the tuples
   *
   * @return a new PyPipe object
   */
  PyPipe notify(bp::object fun);
};

/**
 * @brief PyTopology represents a dataflow graph of operators.
 *
 * PyTopology is the main entry point for constructing a stream processing
 * query in Python. It is used to create pipes with data sources as publishers
 * which can be used to connect other stream operators. PyTopology is just
 * a wrapper around PipeFabric's Topology class for Python.
  *
  * @code
  * import libpfabric
  *
  * t = libpfabric.Topology()
  * p = t.newStreamFromFile("data.csv") \
  *       .extract(',') \
  *       .map(lambda t, o: (int(t[0]), t[1], t[2])) \
  *       .where(lambda x, o: x[0] > 1) \
  *      .print()
  *
  * t.start()
  * @endcode
  */
struct PyTopology {
  /**
   * @brief Creates a new @c PyTopology object.
   */
  PyTopology();

  /**
   * @brief Creates a pipe from a text file source as input.
   *
   * Creates a new pipe for reading tuples (containing only a
   * string field representing a line of the file) via a
   * TextFileSource operator.
   *
   * @param[in] fname
   *    the name of the file from which the tuples are read.
   * @return a new PyPipe object
   */
  PyPipe newStreamFromFile(std::string file);

  /**
   * @brief Starts processing of the whole topology.
   */
  void start();

  /// the PipeFabric context needed for creating a topology
  PFabricContext ctx;
  /// the corresponding topology object
  PFabricContext::TopologyPtr topo;
};
}

#endif
