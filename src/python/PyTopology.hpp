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

#ifndef PyTopology_hpp_
#define PyTopology_hpp_

#include "pfabric.hpp"
#include <boost/python.hpp>
#include <boost/variant.hpp>

namespace bp = boost::python;

namespace pfabric {

enum class AggrFuncType {
    IntSum, DoubleSum,
    Count, DCount,
    IntAvg, DoubleAvg,
    IntMin, DoubleMin, StringMin,
    IntMax, DoubleMax, StringMax,
    IntIdentity, DoubleIdentity, StringIdentity,
    GroupID
};

/// We handle only tuples consisting of a single field that represents
/// a Python tuple.
typedef TuplePtr<bp::object> PyTuplePtr;

class PyAggregateState : public AggregateStateBase<PyTuplePtr> {
public:
  typedef std::shared_ptr<PyAggregateState> AggrStatePtr;

  PyAggregateState() {}
  PyAggregateState(const std::vector<int>& cols, const std::vector<AggrFuncType>& funcs);
  PyAggregateState(const PyAggregateState& s);

  virtual void init();

  static AggrStatePtr create(AggrStatePtr state) {
    PyAggregateState *self = state.get();
    return std::make_shared<PyAggregateState>(PyAggregateState(*self));
  }

  static void iterate(const PyTuplePtr& tp,
    AggrStatePtr state, const bool outdated);

  static void iterateForKey(const PyTuplePtr& tp, const std::string& key,
    AggrStatePtr state, const bool outdated);

  static PyTuplePtr finalize(AggrStatePtr state);

private:
  void setupAggregateFuncs();
  std::vector<int> mColumns;
  std::vector<AggrFuncType> mFuncSpecs;
  std::vector<AggregateFuncBasePtr> mAggrFuncs;
};

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
   * @brief Creates a notify operator for passing stream tuples
   * to a callback function.
   *
   * Creates a notify operator for triggering a callback on each input tuple
   * and forwarding the tuples to the next operator on the pipe.

   * @param[in] func
   *      a lambda function representing the callback that is invoked for
   *      each input tuple
   * @param[in] pfunc
   *      an optional lambda function representing the callback
   *      that is invoked for each punctuation
   * @return a new PyPipe object
   */
  PyPipe notify(bp::object fun);

  /**
   * @brief Defines the timestamp extractor function for all subsequent
   * operators.
   *
   * Defines a function for extracting a timestamp from a tuple which is used
   * for all subsequent operators which require such a function, e.g. windows.
   *
   * @param[in] func
   *      a function for extracting the timestamp of the tuple
   * @return a new pipe
   */
  PyPipe assignTimestamps(bp::object fun);

  /**
   * @brief Defines the key extractor function for all subsequent operators.
   *
   * Defines a function for extracting a key value from a tuple which is used
   * for all subsequent operators which require such a function,
   * e.g. join, groupBy.
   *
   * @param[in] func
   *      a function for extracting the key from the tuple
   * @return a new pipe
   */

  PyPipe keyBy(bp::object fun);

  /**
   * @brief Creates a sliding window operator as the next operator on the pipe.
   *
   * Creates a sliding window operator of the given type and size.
   *
   * @param[in] wt
   *      the type of the window (row or range)
   * @param[in] sz
   *      the window size (in number of tuples for row window or in milliseconds
   *      for range windows)
   * @param[in] ei
   *      the eviction interval, i.e., time for triggering the eviction (in
   *      milliseconds)
   * @return a new pipe
   */
  PyPipe slidingWindow(WindowParams::WinType wt, unsigned int size, unsigned int interval = 0);

  PyPipe aggregate(bp::list columns, bp::list aggrFuncs);

  PyPipe groupBy(bp::list columns, bp::list aggrFuncs);

  PyPipe join(PyPipe other, bp::object predicate);

  PyPipe queue();

  /**
   * @brief Creates a print operator.
   *
   * Creates an operator for printing tuples to the console
   * as the next operator on the pipe.
   *
   * @return a new PyPipe object
   */
  PyPipe print();
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
