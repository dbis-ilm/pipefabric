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
#ifndef Topology_hpp_
#define Topology_hpp_

#include <string>
#include <list>
#include <vector>
#include <future>
#include <mutex>

#include "core/Tuple.hpp"

#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "qop/RESTSource.hpp"
#include "qop/ZMQSource.hpp"
#include "qop/MemorySource.hpp"
#include "qop/ToTable.hpp"
#include "qop/FromTable.hpp"
#include "qop/SelectFromTable.hpp"
#include "qop/StreamGenerator.hpp"

#include "dsl/Pipe.hpp"
#include "dsl/Dataflow.hpp"

namespace pfabric {

  /**
   * @brief A topology represents a dataflow graph of operators.
   *
   * Topology is the main entry point for a stream processing query. It is used
   * to create pipes with data sources as publishers which can be used to connect
   * other stream operators.
   *
   * The following snippet shows an example of using the Topology class.
   *
   * @code
   * // T1 and T2 a typedefs of TuplePtr
   * TopologyPtr t = ctx.createTopology();
   *
   * auto s = t->newStreamFromFile("file.csv")
   *           .extract<T1>(',')
   *           .where<T1>([](auto tp, bool outdated) {
   *                     return getAttribute<0>(*tp) % 2 == 0;
   *            })
   *           .map<T1,T2>([](auto tp) -> T2 {
   *                     return makeTuplePtr(getAttribute<2>(*tp),
   *                                         getAttribute<0>(*tp));
   *            })
   *           .print<T2>(strm);
   * // now, let's start the processing
   * t->start();
   * @endcode
   */
  class Topology {
  private:
    /// the signature of a startup function
    typedef std::function<unsigned long()> StartupFunc;

//    std::list<Pipe*> pipes;               //< the list of pipes created for this topology
    std::vector<StartupFunc> startupList; //< the list of functions to be called for startup
    std::vector<StartupFunc> prepareList; //< the list of functions to be called for startup
    bool asyncStarted;                    //< true if we started asynchronously
    std::vector<std::future<unsigned long> > startupFutures; //< futures for the startup functions
    std::mutex mMutex;                    //< mutex for accessing startupFutures

    DataflowPtr dataflow;

    /**
     * @brief Registers a startup function for initiating the processing.
     *
     * Registers the given function as a startup function of an operator. This is
     * required for all query operators requiring an explicit invocation of a method.
     * A startup function is called and executed asynchronously after @c start
     * is invoked.
     *
     * @param[in] func
     *    a function pointer for the startup member function
     */
    void registerStartupFunction(StartupFunc func);

    void registerPrepareFunction(StartupFunc func);

    /**
     * @brief Invokes the start functions asynchronously.
     */
    void startAsync();

  public:
    /**
     * @brief Constructs a new empty topology.
     */
    Topology() : asyncStarted(false), dataflow(make_shared<Dataflow>()) {}

    /**
     * @brief Destructor for topology.
     */
    ~Topology();

    /**
     * @brief Starts processing of the whole topology.
     *
     * Starts the processing of the topology by invoking the start
     * functions of all operators acting as data source. The start
     * functions can be called either synchronously, i.e. one start
     * function after another, or asynchronously where the functions
     * run in concurrent threads. In both cases, start returns only
     * after all functions are finished.
     *
     * @param[in] async
     *   determines if the start functions should be invoked asynchronously
     */
    void start(bool async = true);

    void prepare();

    /**
     * @brief Waits until the execution of the topology stopped.
     *
     * If the topology was started asynchronously the call of wait()
     * blocks until the execution stopped.
     */
    void wait();

    /**
     * @brief Creates a pipe from a TextFileSource as input.
     *
     * Creates a new pipe for reading tuples (containing only a
     * string field representing a line of the file) via a
     * TextFileSource.
     *
     * @param[in] fname
     *    the name of the file from which the tuples are read.
     * @param[in] limit
     *    maximum number of tuples to be read (default == 0 => read until EOF)
     * @return
     *    a new pipe where TextFileSource acts as a producer.
     */
    Pipe<TStringPtr> newStreamFromFile(const std::string& fname, unsigned long limit = 0);

    /**
     * @brief Creates a pipe from a REST source as input.
     *
     * Creates a new pipe for receiving tuples via a REST server.
     * Each call of the REST service produces a single tuple (consisting
     * of a single string).
     *
     * @param[in] port
     *    the TCP port for receiving REST calls
     * @param[in] path
     *    the local part (path) of the REST URI
     * @param[in] method
     *    the REST method for invoking the service (GET, PUT, POST)
     * @param[in] numThreads
     *    the number of threads to run the service
     * @return
     *    a new pipe where RESTSource acts as a producer.
     */
    Pipe<TStringPtr> newStreamFromREST(unsigned int port,
                            const std::string& path,
                            RESTSource::RESTMethod method,
                            unsigned short numThreads = 1);

    /**
     * @brief Creates a pipe from a ZMQ source as input.
     *
     * Creates a new pipe for receiving tuples via ZMQ and sent them
     * over the stream either as one string (tuple) per message or
     * binary encoded.
     *
     * @param[in] path
     *    the path describing the network connection endpoint, e.g.
     *    tcp://localhost:5678 for a TCP connection at port 5678
     * @param[in] encoding
     *    the encoding used for sending data over ZMQ (ASCII or binary)
     * @param[in] stype
     *    the communication type as provided by ZMQ (Subscriber for PubSub
     *    or Pull).
     * @return
     *    a new pipe where ZMQSource acts as a producer.
     */
    Pipe<TStringPtr> newAsciiStreamFromZMQ(const std::string& path,
      ZMQParams::SourceType stype = ZMQParams::SubscriberSource);

    Pipe<TBufPtr> newBinaryStreamFromZMQ(const std::string& path,
      ZMQParams::SourceType stype = ZMQParams::SubscriberSource);

    /**
     * @brief Creates a pipe for monitoring updates on a table.
     *
     * Creates a new pipe for producing a stream from updates on a
     * table. Each update creates a tuple sent to the stream. Updates
     * can either sent immediately (@c Immediate) or after the commit
     * of an transaction.
     *
     * @tparam T
     *    the record type of the table, usually a @c TuplePtr<Tuple<...> >
     * @tparam KeyType
     *    the data type of the key of the table
     * @param[in] tbl
     *    the table acting as the source for the stream.
     * @param[in] mode
     *    the monitoring mode (@c Immediate or @c OnCommit)
     * @return
     *    a new pipe where RESTSource acts as a producer.
     */
    template<typename T, typename KeyType = DefaultKeyType>
    Pipe<T> newStreamFromTable(std::shared_ptr<Table<typename T::element_type, KeyType>> tbl,
                             TableParams::NotificationMode mode = TableParams::Immediate) {
      auto op = std::make_shared<FromTable<T, KeyType>>(tbl, mode);
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }

    /**
     * @brief Create a new pipe where a named stream is used as input.
     *
     * @tparam T the type of the stream element
     * @param[in] stream
     *    the named stream object from which the pipe receives the data
     * @return
     *    a new pipe where the stream acts as the producer.
     */
    template <typename T>
    Pipe<T> fromStream(Dataflow::BaseOpPtr stream) throw (TopologyException) {
      // check whether stream is a Queue<T> operator
      auto pOp = dynamic_cast<Queue<T>*>(stream.get());
      if (pOp == nullptr) {
        throw TopologyException("Incompatible tuple type of stream object.");
      }
      return Pipe<T>(dataflow, dataflow->addPublisher(stream));
    }

    /**
     * @brief Create a SeletFromTable operator as data source.
     *
     * Create a new SelectFromTable operator that produces a stream of tuples
     * from the given table.
     *
     * @tparam T
     *    the record type of the table, usually a @c TuplePtr<Tuple<...> >
     * @tparam KeyType
     *    the data type of the key of the table
     * @param tbl
     *    the table that is read
     * @param pred
     *    an optional filter predicate
     * @return
     *    a new pipe where the table acts as the source
     */
    template<typename T, typename KeyType = DefaultKeyType>
    Pipe<T> selectFromTable(std::shared_ptr<Table<typename T::element_type, KeyType>> tbl,
        typename Table<typename T::element_type, KeyType>::Predicate pred = nullptr) {
      auto op = std::make_shared<SelectFromTable<T, KeyType>>(tbl, pred);
      registerStartupFunction([=]() -> unsigned long { return op->start(); });
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }

    /**
     * @brief Create a StreamGenerator operator as data source.
     *
     * Create a new StreamGenerator operator that produces a stream of tuples
     * created using the given generator function.
     *
     * @tparam T the type of the stream element
     * @param gen
     *    a generator function for creating the tuples
     * @param num
     *    the number of tuples to be created
     * @return
     *    a new pipe where the generator acts as the source
     */
    template<typename T>
    Pipe<T> streamFromGenerator(typename StreamGenerator<T>::Generator gen, unsigned long num) {
      auto op = std::make_shared<StreamGenerator<T>>(gen, num);
      registerStartupFunction([=]() -> unsigned long { return op->start(); });
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }

    template<typename T>
    Pipe<T> newStreamFromMemory(const std::string& fname, char delim = ',', unsigned long num = 0) {
      auto op = std::make_shared<MemorySource<T>>(fname, delim, num);
      registerStartupFunction([=]() -> unsigned long { return op->start(); });
      registerPrepareFunction([=]() -> unsigned long { return op->prepare(); });
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }
  };

}

#endif
