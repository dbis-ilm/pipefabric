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

#pragma once
#ifndef Topology_hpp_
#define Topology_hpp_

#include <chrono>
#include <condition_variable>
#include <string>
#include <list>
#include <vector>
#include <future>
#include <mutex>

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

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
#include "qop/FromTxTables.hpp"
#include "qop/SelectFromTable.hpp"
#include "qop/SelectFromTxTable.hpp"
#include "qop/SelectFromMVCCTable.hpp"
#include "qop/StreamGenerator.hpp"
#ifdef SUPPORT_MATRICES
  #include "qop/FromMatrix.hpp"
#endif
#include "dsl/Pipe.hpp"
#include "dsl/Dataflow.hpp"
#ifdef USE_RABBITMQ
  #include "net/RabbitMQSource.hpp"
#endif
#ifdef USE_KAFKA
  #include "net/KafkaSource.hpp"
#endif
#ifdef USE_MQTT
  #include "net/MQTTSource.hpp"
#endif
#ifdef BUILD_USE_CASES
  #include "usecases/LinearRoad/DataDriverLR.hpp"
#endif

using namespace std::chrono_literals;

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
   *                     return get<0>(tp) % 2 == 0;
   *            })
   *           .map<T1,T2>([](auto tp, bool) -> T2 {
   *                     return makeTuplePtr(get<2>(tp),
   *                                         get<0>(tp));
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

    std::vector<StartupFunc> startupList; //< the list of functions to be called for startup
    std::vector<StartupFunc> prepareList; //< the list of functions to be called for startup
    bool asyncStarted;                    //< true if we started asynchronously
    std::vector<std::future<unsigned long> > startupFutures; //< futures for the startup functions
    std::vector<boost::thread> wakeupTimers; //< interruptible threads for runEvery queries
    std::mutex mMutex;                    //< mutex for accessing startupFutures
    std::condition_variable mCv;          //< condition variable to check if sinks have received EndOfStream
    std::mutex mCv_m;

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
    Topology() : asyncStarted(false), dataflow(std::make_shared<Dataflow>()) {}

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
     * @brief Runs the topology periodically every @c secs seconds.
     *
     * Starts the processing of the topology every @c secs seconds. Note,
     * that the topology should be a finite query not a continuous stream
     * query.
     *
     * @param[in] secs
     *  the period of time between two invocations
     */
    void runEvery(unsigned long secs);

    void cleanStartupFunctions();
    void stopThreads();

    /**
     * @brief Waits until the execution of the topology stopped.
     *
     * If the topology was started asynchronously the call of wait()
     * blocks until the execution stopped.
     */
    void wait(const std::chrono::milliseconds &dur = 500ms);

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

#ifdef USE_RABBITMQ
    /**
     * @brief Creates a pipe from a RabbitMQ source as input.
     *
     * Creates a new pipe for receiving tuples via AMQP server (RabbitMQ).
     * It reads messages from the AMQP queue and forwards them as tuples
     * to the subscribers, as long as there are messages on the server.
     *
     * @param[in] info
     *    a string containing password, user, address and port of the server
     *    format: "password:user@address:port", e.g. "guest:guest@localhost:5672"
     * @param[in] queueName
     *    a string containing the name of the queue for exchanging tuples, e.g. "queue"
     * @return
     *    a new pipe where RabbitMQSource acts as a producer.
     */
    Pipe<TStringPtr> newStreamFromRabbitMQ(const std::string& info, const std::string& queueName);
#endif

#ifdef USE_KAFKA
    /**
     * @brief Creates a pipe from an Apache Kafka source as input.
     *
     * Creates a new pipe for receiving tuples via Apache Kafka protocol.
     *
     * @param[in] broker
     *    the node(s) where the Kafka server runs on,
     *    e.g. "127.0.0.1:9092" for localhost
     * @param[in] topic
     *    the topic where the data is stored (Kafka property)
     * @param[in] groupID
     *    the ID of the group the consumer belongs to
     * @return
     *    a new pipe where KafkaSource acts as a producer.
     */
    Pipe<TStringPtr> newStreamFromKafka(const std::string& broker, const std::string& topic,
                                        const std::string& groupID);
#endif

#ifdef USE_MQTT
    /**
     * @brief Creates a pipe from a MQTT source as input.
     *
     * Creates a new pipe for receiving tuples via MQTT.
     *
     * @param[in] conn
     *    server connection info, e.g. "tcp://localhost:1883"
     * @param[in] channel
     *    the name of the channel to listen on
     * @return
     *    a new pipe where MQTTSource acts as a producer.
     */
    Pipe<TStringPtr> newStreamFromMQTT(const std::string& conn, const std::string& channel);
#endif

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
      const std::string& syncPath = "",
      ZMQParams::SourceType stype = ZMQParams::SubscriberSource);

    Pipe<TBufPtr> newBinaryStreamFromZMQ(const std::string& path,
      const std::string& syncPath = "",
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

#ifdef SUPPORT_MATRICES
    /**
     * @brief Create a pipe for stream from matrix
     * @tparam Matrix
     *   matrix type
     * @tparam Matrix::StreamElement
     *   record type of the matrix like @c TuplePtr< int, int, double >
     * @param[in] matrix
     *   the matrix is source of stream.
     * @return
     *   a new pipe with new stream.
     */
    template<typename Matrix>
    Pipe<typename Matrix::StreamElement> newStreamFromMatrix(std::shared_ptr<Matrix> matrix) {
      auto op = std::make_shared<FromMatrix< Matrix > >(matrix);
      return Pipe<typename Matrix::StreamElement>(dataflow, dataflow->addPublisher(op));
    }
#endif

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
    Pipe<T> fromStream(Dataflow::BaseOpPtr stream) noexcept(false) {
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

    template<typename T, typename KeyType = DefaultKeyType>
    Pipe<T> selectFromTxTable(std::shared_ptr<TxTable<typename T::element_type, KeyType>> tbl,
        typename TxTable<typename T::element_type, KeyType>::Predicate pred = nullptr) {
      auto op = std::make_shared<SelectFromTxTable<T, KeyType>>(tbl, pred);
      registerStartupFunction([=]() -> unsigned long { return op->start(); });
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }

    template<typename T, typename KeyType = DefaultKeyType>
    Pipe<T> selectFromMVCCTable(
      std::shared_ptr<MVCCTable<typename T::element_type, KeyType>> tbl,
      std::atomic<TransactionID>& aCnter,
      typename MVCCTable<typename T::element_type,
                         KeyType>::Predicate pred = nullptr) {
      auto op = std::make_shared<SelectFromMVCCTable<T, KeyType>>(tbl, aCnter, pred);
      registerStartupFunction([=]() -> unsigned long { return op->start(); });
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }

    template<typename TableType, typename T, size_t TxSize>
    Pipe<T> fromTxTables(StateContext<TableType>& sCtx) {
      auto op = std::make_shared<FromTxTables<TableType, T, TxSize>>(sCtx);
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

#ifdef BUILD_USE_CASES
    //Linear Road data producer
    template<typename T>
    Pipe<T> newStreamFromLinRoad(const std::string& fname) {
      auto op = std::make_shared<DataDriverLR<T>>(fname);
      registerStartupFunction([=]() -> unsigned long { return op->start(); });
      return Pipe<T>(dataflow, dataflow->addPublisher(op));
    }
#endif
  };

}

#endif
