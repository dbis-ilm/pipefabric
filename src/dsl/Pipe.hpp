/*
 * Copyright (c) 2014-16 The PipeFabric team,
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
#ifndef Pipe_hpp_
#define Pipe_hpp_

#include <string>

#include <typeinfo>

#include <boost/any.hpp>

#include "core/Tuple.hpp"
#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "qop/Where.hpp"
#include "qop/Notify.hpp"
#include "qop/Queue.hpp"
#include "qop/Map.hpp"
#include "qop/StatefulMap.hpp"
#include "qop/TupleExtractor.hpp"
#include "qop/JsonExtractor.hpp"
#include "qop/TupleDeserializer.hpp"
#include "qop/ConsoleWriter.hpp"
#include "qop/FileWriter.hpp"
#include "qop/SlidingWindow.hpp"
#include "qop/TumblingWindow.hpp"
#include "qop/Aggregation.hpp"
#include "qop/GroupedAggregation.hpp"
#include "qop/SHJoin.hpp"
#include "qop/ToTable.hpp"
#include "qop/Merge.hpp"
#include "qop/PartitionBy.hpp"
#include "qop/Barrier.hpp"
#include "qop/Batcher.hpp"
#include "cep/Matcher.hpp"
#include "qop/ZMQSink.hpp"
#include "dsl/Dataflow.hpp"
#include "dsl/TopologyException.hpp"

namespace pfabric {

    enum PartitioningState {
        NoPartitioning,
        FirstInPartitioning,
        NextInPartitioning
    };


  /**
   * @brief Pipe represents a sequence of operators applied to a data stream. Pipes are used
   *        mainly to construct a dataflow programatically.
   *
   * A Pipe is used to construct and represent a dataflow program. Pipes are constructed by
   * creating a new data source via the @c Topology class. Then, new operators can be added one
   * by one via methods of the @c Pipe class.
   */
  template <typename T>
  class Pipe {
  private:
    friend class Topology;
    PartitioningState partitioningState;

    typedef Dataflow::BaseOpIterator OpIterator;

    /// Note, we need boost::any values here because the functions pointers are typed (via templates)
    boost::any timestampExtractor; // a function pointer to a timestamp extractor function
    boost::any keyExtractor;            // a function pointer to a key extractor function
    unsigned int numPartitions;
    DataflowPtr dataflow;
    OpIterator tailIter;

    /**
     * @brief Constructor for Pipe.
     *
     * Creates a new pipe with the given operator @c op as initial publisher.
     *
     * @param[in] op
     *     an operator producing the tuple for the this pipeline
     */
    Pipe(DataflowPtr ptr, Dataflow::BaseOpIterator iter) :
      partitioningState(NoPartitioning), numPartitions(0), dataflow(ptr), tailIter(iter) {
    }

   public:
    Pipe(DataflowPtr ptr, Dataflow::BaseOpIterator iter, boost::any keyFunc, boost::any tsFunc, PartitioningState pState,
         unsigned int nPartitions = 0) : partitioningState(pState),
          timestampExtractor(tsFunc), keyExtractor(keyFunc),
          numPartitions(nPartitions), dataflow(ptr), tailIter(iter) {
    }

    Pipe(const Pipe<T>& p) {
      partitioningState = p.partitioningState;
      numPartitions = p.numPartitions;
      dataflow = p.dataflow;
      tailIter = p.tailIter;
    }

  private:
    /**
     * @brief Returns the operator at the end of the publisher list.
     *
     * Returns the operator which acts as the publisher for the next
     * added operator.
     *
     * @return
     *    the last operator in the publisher list
     */
    Dataflow::BaseOpPtr getPublisher() {
      BOOST_ASSERT_MSG(tailIter != dataflow->publisherEnd(), "No DataSource available in dataflow");
      return *tailIter;
    }

    Dataflow::BaseOpIterator getPublishers() {
      return tailIter;
    }

    template<typename SourceType>
    SourceType* castOperator(Dataflow::BaseOpPtr opPtr) throw (TopologyException) {
      auto pOp = dynamic_cast<SourceType*>(opPtr.get());
      if (pOp == nullptr) {
        throw TopologyException("Incompatible tuple types in Pipe.");
      }
      return pOp;
    }

    template<typename SourceType>
    SourceType* castOperator(BaseOp *opPtr) throw (TopologyException) {
      auto pOp = dynamic_cast<SourceType*>(opPtr);
      if (pOp == nullptr) {
        throw TopologyException("Incompatible tuple types in Pipe.");
      }
      return pOp;
    }

    template<typename Publisher, typename SourceType>
    OpIterator addPublisher(std::shared_ptr<Publisher> op) throw (TopologyException) {
      auto pOp = castOperator<SourceType>(getPublisher());
      CREATE_LINK(pOp, op);
      return dataflow->addPublisher(op);
    }

    template<typename Publisher, typename StreamElement>
    OpIterator addPartitionedPublisher(std::vector<std::shared_ptr<Publisher>>& opList)
      throw (TopologyException) {
      if (partitioningState == NoPartitioning)
        throw TopologyException("Missing partitionBy operator in topology.");

      if (partitioningState == FirstInPartitioning) {
        auto partition = castOperator<PartitionBy<StreamElement>>(getPublisher());

        for (auto i = 0u; i < opList.size(); i++) {
          auto op = opList[i];
          partition->connectChannelsForPartition(i,
            op->getInputDataChannel(),
            op->getInputPunctuationChannel());
        }
        partitioningState = NextInPartitioning;
      }
      else {
        auto iter = getPublishers();
        for (auto i = 0u; i < opList.size() && iter != dataflow->publisherEnd(); i++) {
          auto p = iter->get();
          auto pOp = castOperator<DataSource<StreamElement>>(p);
          CREATE_LINK(pOp, opList[i]);
          iter++;
        }
      }
      Dataflow::BaseOpList bops(opList.begin(), opList.end());
      return dataflow->addPublisherList(bops);
    }

  public:
    /**
     * @brief Destructor of the pipe.
     *
     * Destroys the pipe and removes all publishers.
     */
    ~Pipe() {}

    /**
     * @brief Defines the key extractor function for all subsequent operators.
     *
     * Defines a function for extracting a key value from a tuple which is used
     * for all subsequent operators which require such a function,
     * e.g. join, groupBy.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam KeyType
     *      the data type of the key
     * @param[in] func
     *      a function pointer to a function extracting the key from the tuple
     * @return a new pipe
     */
    template <typename KeyType = DefaultKeyType>
    Pipe<T> keyBy(std::function<KeyType(const T&)> func) {
      return Pipe<T>(dataflow, tailIter, func, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Defines the key column for all subsequent operators.
     *
     * Defines a column representing the key in a tuple which is used
     * for all subsequent operators which require such a function,
     * e.g. join, groupBy.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam N
     *      the index of column used as key
     * @tparam KeyType
     *      the data type of the key
     * @return a new pipe
     */
    template <int N, typename KeyType = DefaultKeyType>
    Pipe<T> keyBy() {
      std::function<KeyType(const T&)> func =
        [](const T& tp) -> KeyType { return getAttribute<N>(tp); };
      return Pipe<T>(dataflow, tailIter, func,
        timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Defines the timestamp extractor function for all subsequent operators.
     *
     * Defines a function for extracting a timestamp from a tuple which is used
     * for all subsequent operators which require such a function, e.g. windows.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] func
     *      a function pointer to a function extracting the timestamp of the tuple
     * @return a new pipe
     */
    Pipe<T> assignTimestamps(typename Window<T>::TimestampExtractorFunc func) {
      return Pipe<T>(dataflow, tailIter, keyExtractor, func, partitioningState, numPartitions);
    }

    /**
     * @brief Defines the timestamp column for all subsequent operators.
     *
     * Defines a column representing the timestamp in a tuple which is used
     * for all subsequent operators which require such a function,
     * e.g. window.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam N
     *      the index of column used as key
     * @return a new pipe
     */
    template <int N>
    Pipe<T> assignTimestamps() {
      std::function<Timestamp(const T&)> func =
        [](const T& tp) -> Timestamp { return getAttribute<N>(tp); };
      return Pipe<T>(dataflow, tailIter, keyExtractor, func, partitioningState, numPartitions);
    }

    /**
     * @brief Creates a sliding window operator as the next operator on the pipe.
     *
     * Creates a sliding window operator of the given type and size.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] wt
     *      the type of the window (row or range)
     * @param[in] sz
     *      the window size (in number of tuples for row window or in milliseconds for range
     *      windows)
     * @param[in] ei
     *      the eviction interval, i.e., time for triggering the eviction (in milliseconds)
     * @return a new pipe
     */
    Pipe<T> slidingWindow(const WindowParams::WinType& wt,
                        const unsigned int sz,
                        const unsigned int ei = 0) throw (TableException) {
      typedef typename Window<T>::TimestampExtractorFunc ExtractorFunc;
      assert(partitioningState == NoPartitioning);

      // we use a try block because the type cast of the timestamp extractor
      // could fail
      try {
        ExtractorFunc fn;
        std::shared_ptr<SlidingWindow<T> > op;

        if (wt == WindowParams::RangeWindow) {
          // a range window requires a timestamp extractor
          fn = boost::any_cast<ExtractorFunc>(timestampExtractor);
          op = std::make_shared<SlidingWindow<T> >(fn, wt, sz, ei);
        }
        else
          op =  std::make_shared<SlidingWindow<T> >(wt, sz, ei);
        auto iter = addPublisher<SlidingWindow<T>, DataSource<T>>(op);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
      catch (boost::bad_any_cast &e) {
        throw TopologyException("No TimestampExtractor defined for slidingWindow.");
      }
    }

    /**
     * @brief Creates a tumbling window operator as the next operator on the pipe.
     *
     * Creates a tumbling window operator of the given type and size.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] wt
     *      the type of the window (row or range)
     * @param[in] sz
     *      the window size (in number of tuples for row window or in milliseconds for range
     *      windows)
     * @return a new pipe
     */
    Pipe<T> tumblingWindow(const WindowParams::WinType& wt,
                         const unsigned int sz) throw (TableException) {
      typedef typename Window<T>::TimestampExtractorFunc ExtractorFunc;
      assert(partitioningState == NoPartitioning);

      try {
        ExtractorFunc fn;
        std::shared_ptr<TumblingWindow<T> > op;

        if (wt == WindowParams::RangeWindow) {
          // a range window requires a timestamp extractor
          fn = boost::any_cast<ExtractorFunc>(timestampExtractor);
          op = std::make_shared<TumblingWindow<T> >(fn, wt, sz);
        }
        else
          op = std::make_shared<TumblingWindow<T> >(wt, sz);
        auto iter = addPublisher<TumblingWindow<T>, DataSource<T>>(op);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
      catch (boost::bad_any_cast &e) {
        throw TopologyException("No TimestampExtractor defined for tumblingWindow.");
      }
    }

    /**
     * @brief Creates a print operator (ConsoleWriter) with an optional user-defined formatting function
     *        as the next operator on the pipe.
     *
     * Create a operator which prints all incoming tuples to the given ostream
     * (usually std::cout or a stringstream) possibly with a user-defined
     * formatting function.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] os
     *      the output stream (std::cout, a stringstream)
     * @param[in] ffun
     *      an optional formatting function producing a string from the tuple
     * @return a new pipe
     */
    Pipe<T> print(std::ostream& os = std::cout,
                typename ConsoleWriter<T>::FormatterFunc ffun = ConsoleWriter<T>::defaultFormatter)
                throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      auto op = std::make_shared<ConsoleWriter<T>>(os, ffun);
      auto pOp = castOperator<DataSource<T>>(getPublisher());
      CREATE_LINK(pOp, op);
      // we don't save the operator in publishers because ConsoleWriter
      // cannot act as Publisher
      dataflow->addSink(op);
      return Pipe<T>(dataflow, tailIter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Creates an operator for saving tuples to a file.
     *
     * Creates an operator for saving tuples to a file with the given name
     * as the next operator on the pipe.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] fname
     *      the name of the output file
     * @param[in] ffun
     *      an optional formatting function producing a string from the tuple
     * @return a new pipe
     */
    Pipe<T> saveToFile(const std::string& fname,
                     typename FileWriter<T>::FormatterFunc ffun = ConsoleWriter<T>::defaultFormatter)
                     throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      auto op = std::make_shared<FileWriter<T> >(fname, ffun);
      auto pOp = castOperator<DataSource<T>>(getPublisher());
        CREATE_LINK(pOp, op);
      // we don't save the operator in publishers because FileWriter
      // cannot act as Publisher
      dataflow->addSink(op);
      return Pipe<T>(dataflow, tailIter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Creates an operator for sending tuples via 0MQ.
      *
     * Creates an operator for sending tuples via 0MQ to another node.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in]path
     *      the path (endpoint) describing the socket (see 0MQ documentation for details)
     * @param[in] type
     *      the type of communication pattern (publish-subscribe, push-pull)
     * @param[in]
     *      mode the encoding mode for messages (binary, ascii, ...)
     * @return a new pipe
     */
   Pipe<T> sendZMQ(const std::string& path, ZMQParams::SinkType stype = ZMQParams::PublisherSink,
       ZMQParams::EncodingMode mode = ZMQParams::BinaryMode)
       throw (TopologyException) {
         assert(partitioningState == NoPartitioning);
      auto op = std::make_shared<ZMQSink<T> >(path, stype, mode);
      auto pOp = castOperator<DataSource<T>>(getPublisher());
      CREATE_LINK(pOp, op);
      // we don't save the operator in publishers because ZMQSink
      // cannot act as Publisher
      dataflow->addSink(op);
     return Pipe<T>(dataflow, tailIter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Creates an data extraction operator.
     *
     * Creates an operator for extracting typed fields from a simple string tuple
     * as the next operator on the pipe.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] sep
     *      the field separator (a single character)
     * @return a new pipe
     */
    template <class Tout>
    Pipe<Tout> extract(char sep) throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      auto op = std::make_shared<TupleExtractor<Tout> >(sep);
      auto iter = addPublisher<TupleExtractor<Tout>, DataSource<TStringPtr> >(op);
      return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Creates an data extraction operator.
     *
     * Creates an operator for extracting typed fields from a JSON string tuple
     * as the next operator on the pipe.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] keys
     * @return a new pipe
     */
    template <class Tout>
    Pipe<Tout> extractJson(const std::initializer_list<std::string>& keys) throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      std::vector<std::string> keyList(keys);
      auto op = std::make_shared<JsonExtractor<Tout> >(keyList);
      auto iter = addPublisher<JsonExtractor<Tout>, DataSource<TStringPtr> >(op);
      return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * TODO
     */
    Pipe<BatchPtr<T>> batch(std::size_t bsize) throw (TopologyException) {
      auto op = std::make_shared<Batcher<T> >(bsize);
      auto iter = addPublisher<Batcher<T>, DataSource<T> >(op);
      return Pipe<BatchPtr<T>>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }
    /**
     * @brief
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @return a new pipe
     */
    template <class Tout>
    Pipe<Tout> deserialize() throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      auto op = std::make_shared<TupleDeserializer<Tout> >();
      auto iter = addPublisher<TupleDeserializer<Tout>, DataSource<TBufPtr> >(op);
      return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

   /**
     * @brief Creates a filter operator for selecting tuples.
     *
     * Creates a filter operator which forwards only tuples satisfying the given filter predicate
     * as the next operator on the pipe.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] func
     *      a function pointer or lambda function implementing a predicate by returning a @c bool
     *      value for the input tuple
     * @return a new pipe
     */
    Pipe<T> where(typename Where<T>::PredicateFunc func) throw (TopologyException) {
      if (partitioningState == NoPartitioning) {
        auto op = std::make_shared<Where<T> >(func);
        auto iter = addPublisher<Where<T>, DataSource<T> >(op);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
      else {
        std::vector<std::shared_ptr<Where<T>>> ops;
        for (auto i = 0u; i < numPartitions; i++) {
          ops.push_back(std::make_shared<Where<T> >(func));
        }
        auto iter = addPartitionedPublisher<Where<T>, T>(ops);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
    }

    /**
      * @brief Creates a notify operator for passing stream tuples to a callback function.
      *
      * Creates a notify operator for triggering a callback on each input tuple and
      * forwarding the tuples to the next operator on the pipe.
      *
      * @tparam T
      *      the input tuple type (usually a TuplePtr) for the operator.
      * @param[in] func
      *      a function pointer or lambda function representing the callback
      *      that is invoked for each input tuple
      * @param[in] pfunc
      *      an optional function pointer or lambda function representing the callback
      *      that is invoked for each punctuation
      * @return a new pipe
      */
     Pipe<T> notify(typename Notify<T>::CallbackFunc func,
                 typename Notify<T>::PunctuationCallbackFunc pfunc = nullptr) throw (TopologyException) {
        assert(partitioningState == NoPartitioning);

       auto op = std::make_shared<Notify<T> >(func, pfunc);
       auto iter = addPublisher<Notify<T>, DataSource<T> >(op);
       return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
     }

    /**
     * @brief Creates a queue operator for decoupling operators.
     *
     * Creates a queue operator which allows to decouple two operators in the
     * dataflow. The upstream part inserts tuples into the queue which is
     * processed by a separate thread to retrieve tuples from the queue and sent
     * them downstream. In this way, the upstream part is not blocked anymore.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @return a new pipe
     */
    Pipe<T> queue() throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      auto op = std::make_shared<Queue<T> >();
      auto iter = addPublisher<Queue<T>, DataSource<T> >(op);
      return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

   /**
    * @brief Creates an operator the forwards the tuples to a named stream object.
    *
    * Creates an operator the forwards all tuples to a named stream object that
    * was created before explicitly via @c PFabricContext.
    *
    * @tparam T
    *      the input tuple type for the operator.
    * @param stream
    *      the named stream object to which the tuples are sent
    * @return a new pipe
    */
    Pipe<T> toStream(Dataflow::BaseOpPtr stream) throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      auto queueOp = castOperator<Queue<T>>(stream);
      auto pOp = castOperator<DataSource<T>>(getPublisher());
      CREATE_LINK(pOp, queueOp);
      return Pipe<T>(dataflow, tailIter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Creates a projection operator.
     *
     * Creates a map operator which applies a mapping (projection) function to each tuples
     * as the next operator on the pipe.
     *
     * @tparam Tin
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam Tout
     *      the result tuple type (usually a TuplePtr) for the operator.
     * @param[in] func
     *      a function pointer or lambda function accepting a tuple of type @c Tin and creating
     *      a new tuple of type @c Tout
     * @return new pipe
     */
    template <typename Tout>
    Pipe<Tout> map(typename Map<T, Tout>::MapFunc func) throw (TopologyException) {
      if (partitioningState == NoPartitioning) {
        auto op = std::make_shared<Map<T, Tout> >(func);
        auto iter = addPublisher<Map<T, Tout>, DataSource<T> >(op);
        return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);

      }
      else {
        std::vector<std::shared_ptr<Map<T, Tout>>> ops;
        for (auto i = 0u; i < numPartitions; i++) {
          ops.push_back(std::make_shared<Map<T, Tout>>(func));
        }
        auto iter = addPartitionedPublisher<Map<T, Tout>, T>(ops);
        return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
    }

    /**
     * @brief Creates a stateful map operator.
     *
     * A StatefulMap operator produces tuples according to a given map function by
     * incorporating a state which is modified inside the map function.
     *
     * @tparam Tin
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam Tout
     *      the result tuple type (usually a TuplePtr) for the operator.
     * @tparam StateRep
     *    the class for representing the state
     * @param[in] func
     *      a function pointer or lambda function accepting a tuple of type @c Tin and creating
     *      a new tuple of type @c Tout
     * @return new pipe
     */
    template <typename Tout, typename State>
    Pipe<Tout> statefulMap(typename StatefulMap<T, Tout, State>::MapFunc func) throw (TopologyException) {
      if (partitioningState == NoPartitioning) {
        auto op = std::make_shared<StatefulMap<T, Tout, State> >(func);
        auto iter = addPublisher<StatefulMap<T, Tout, State>, DataSource<T> >(op);
        return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);

      }
      else {
        std::vector<std::shared_ptr<StatefulMap<T, Tout, State>>> ops;
        for (auto i = 0u; i < numPartitions; i++) {
          ops.push_back(std::make_shared<StatefulMap<T, Tout, State>>(func));
        }
        auto iter = addPartitionedPublisher<StatefulMap<T, Tout, State>, T>(ops);
        return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
    }

    /*------------------------ grouping and aggregation -----------------------*/

    /**
     * @brief Creates an operator for calculating aggregates over the entire stream.
     *
     * Creates an operator for calculating a set of aggregates over the stream,
     * possibly supported by a window. Depending on the parameters each input
     * tuple triggers the calculating and produces a new aggregate value which
     * is forwarded as a result tuple.
     * The following example illustrates the usage of this method with the
     * helper template classes Aggregator1:
     * @code
     * // calculate the sum of column #0
     * typedef Aggregator1<T1, AggrSum<double>, 0> MyAggrState;
     *
     * // Aggregator1 defines already functions for finalize and iterate
     * t->newStreamFrom...
     *    .aggregate<T1, T2, MyAggrState> ()
     * @endcode
     *
     * @tparam Tin
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam Tout
     *      the result tuple type (usually a TuplePtr) for the operator.
     * @tparam AggrState
     *      the type of representing the aggregation state as a subclass of
     *      @c AggregationStateBase. There are predefined template classes
     *      @c Aggregator1 ... @c AggregatorN which can be used directly here.
     * @param[in] tType
     *    the mode for triggering the calculation of the aggregate (TriggerAll,
     *    TriggerByCount, TriggerByTime, TriggerByTimestamp)
     * @param[in] tInterval
     *    the interval for producing aggregate tuples
     * @return a new pipe
     */
    template <typename Tout, typename AggrState>
    Pipe<Tout> aggregate(AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0)
      throw (TopologyException) {
      return aggregate<Tout, AggrState>(AggrState::finalize, AggrState::iterate, tType, tInterval);
    }

    /**
      * @brief Creates an operator for calculating aggregates over the entire stream.
      *
      * Creates an operator for calculating a set of aggregates over the stream,
      * possibly supported by a window. Depending on the parameters each input
      * tuple triggers the calculating and produces a new aggregate value which
      * is forwarded as a result tuple. The difference to the other aggregate
      * function is that this method allows to specify the finalize and iterate
      * method.
      *
      * The following example illustrates the usage of this method with the
      * helper template classes Aggregator1:
      * @code
      * // calculate the sum of column #0
      * typedef Aggregator1<T1, AggrSum<double>, 0> MyAggrState;
      * typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
      *
      * // Aggregator1 defines already functions for finalize and iterate
      * t->newStreamFrom...
      *    .aggregate<T1, T2, MyAggrState> (MyAggrState::finalize,
      *                                    MyAggrState::iterate)
      * @endcode
      *
      * @tparam Tin
      *      the input tuple type (usually a TuplePtr) for the operator.
      * @tparam Tout
      *      the result tuple type (usually a TuplePtr) for the operator.
      * @tparam AggrState
      *      the type of representing the aggregation state as a subclass of
      *      @c AggregationStateBase. There are predefined template classes
      *      @c Aggregator1 ... @c AggregatorN which can be used directly here.
      * @param[in] finalFun
      *    a function pointer for constructing the aggregation tuple
      * @param[in] iterFun
      *    a function pointer for increment/decrement (in case of outdated tuple)
      *    the aggregate values.
      * @param[in] tType
      *    the mode for triggering the calculation of the aggregate (TriggerAll,
      *    TriggerByCount, TriggerByTime, TriggerByTimestamp)
      * @param[in] tInterval
      *    the interval for producing aggregate tuples
      * @return a new pipe
      */
      template <typename Tout, typename AggrState>
      Pipe<Tout> aggregate(typename Aggregation<T, Tout, AggrState>::FinalFunc finalFun,
                     typename Aggregation<T, Tout, AggrState>::IterateFunc iterFun,
                     AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0)
                     throw (TopologyException) {
        assert(partitioningState == NoPartitioning);
       auto op = std::make_shared<Aggregation<T, Tout, AggrState> >(finalFun, iterFun,
            tType, tInterval);
       auto iter = addPublisher<Aggregation<T, Tout, AggrState>, DataSource<T> >(op);
       return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
     }

     /**
      * @brief Creates an operator for calculating grouped aggregates over the entire stream.
      *
      * Creates an operator implementing a groupBy together with aggregations which
      * are represented internally by instances of AggregateState. The operator supports
      * window-based aggregation by handling delete tuples accordingly.
      *
      * @tparam Tin
      *      the input tuple type (usually a TuplePtr) for the operator.
      * @tparam Tout
      *      the result tuple type (usually a TuplePtr) for the operator.
      *      the type of representing the aggregation state as a subclass of
      *      @c AggregationStateBase. There are predefined template classes
      *      @c Aggregator1 ... @c AggregatorN which can be used directly here.
      * @tparam KeyType
      *      the data type for representing keys (grouping values)
      * @param[in] tType
      *    the mode for triggering the calculation of the aggregate (TriggerAll,
           TriggerByCount, TriggerByTime, TriggerByTimestamp)
      * @param[in] tInterval
      *    the interval for producing aggregate tuples
      * @return a new pipe
      */
     /*template <typename Tout, typename AggrState, typename KeyType = DefaultKeyType>
     Pipe<Tout>& groupBy(AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0)
      throw (TopologyException) {
       return groupBy<Tout, AggrState, KeyType>(AggrState::finalize, AggrState::iterate, tType, tInterval);
    }*/

    /**
     * @brief Creates an operator for calculating grouped aggregates over the entire stream.
     *
     * Creates an operator implementing a groupBy together with aggregations which
     * are represented internally by instances of AggregateState. The operator supports
     * window-based aggregation by handling delete tuples accordingly.
     *
     * @tparam Tin
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam Tout
     *      the result tuple type (usually a TuplePtr) for the operator.
     *      the type of representing the aggregation state as a subclass of
     *      @c AggregationStateBase. There are predefined template classes
     *      @c Aggregator1 ... @c AggregatorN which can be used directly here.
     * @tparam KeyType
     *      the data type for representing keys (grouping values)
     * @param[in] aggrStatePtr
     *      an instance of the AggrState class which is used as prototype
     * @param[in] finalFun
     *    a function pointer for constructing the aggregation tuple
     * @param[in] iterFun
     *    a function pointer for increment/decrement (in case of outdated tuple)
     *    the aggregate values.
     * @param[in] tType
     *    the mode for triggering the calculation of the aggregate (TriggerAll,
          TriggerByCount, TriggerByTime, TriggerByTimestamp)
     * @param[in] tInterval
     *    the interval for producing aggregate tuples
     * @return a new pipe
     */
    template <typename Tout, typename AggrState, typename KeyType = DefaultKeyType>
    Pipe<Tout> groupBy(//std::shared_ptr<AggrState> aggrStatePtr,
                    //typename GroupedAggregation<T, Tout, AggrState, KeyType>::FinalFunc finalFun,
                    //typename GroupedAggregation<T, Tout, AggrState, KeyType>::IterateFunc iterFun,
                    AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0)
                    throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      try {
        typedef std::function<KeyType(const T&)> KeyExtractorFunc;
        KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);

        auto op =
          std::make_shared<GroupedAggregation<T, Tout, AggrState, KeyType> >(//aggrStatePtr,
              //keyFunc, finalFun, iterFun, tType, tInterval);
		keyFunc, AggrState::finalize, AggrState::iterate, tType, tInterval);
        auto iter = addPublisher<GroupedAggregation<T, Tout, AggrState, KeyType>, DataSource<T> >(op);
        return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      } catch (boost::bad_any_cast &e) {
        throw TopologyException("No KeyExtractor defined for groupBy.");
      }
    }

    /*----------------------------------- CEP ---------------------------------*/

    /**
     * @brief Creates an operator for pattern detection over the stream
     * using NFA concept.
     *
     * Creates an operator implementing the matcher operator to
     * detect complex events and patterns over the stream. The operator
     * uses the NFA concept to carry out its task.
     *
     * @tparam Tin
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam Tout
     *      the result tuple type (usually a TuplePtr) for the operator.
     *      the type of representing the matcher result
     * @tparam Dependency
     *      the type of related values
     * @param[in] nfa
     *      an instance of our working NFA
     * @return a reference to the pipe
     * TODO: make better
     */
    template <typename Tout, typename RelatedValueType>
    Pipe<Tout> matchByNFA(typename NFAController<T, Tout, RelatedValueType>::NFAControllerPtr nfa) throw (TopologyException) {
    	auto op = std::make_shared<Matcher<T, Tout, RelatedValueType>>(
				Matcher<T, Tout, RelatedValueType>::FirstMatch);
    	op->setNFAController(nfa);
      auto iter = addPublisher<Matcher<T, Tout, RelatedValueType>, DataSource<T> >(op);
      return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /**
     * @brief Creates an operator for pattern detection over the stream
     * using NFA concept.
     *
     * Creates an operator implementing the matcher operator to
     * detect complex events and patterns over the stream. The operator
     * uses the NFA concept to carry out its task.
     *
     * @tparam Tin
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam Tout
     *      the result tuple type (usually a TuplePtr) for the operator.
     *      the type of representing the matcher result
     * @tparam Dependency
     *      the type of related values
     * @param[in] expr
     *      an expression for event matching using sequence operator (>>),
     *      or operator (||) and negation (!)
     * @return a reference to the pipe
     */
    template <typename Tout, typename RelatedValueType>
    Pipe<Tout> matcher(CEPState<T, RelatedValueType>& expr) throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
    	auto op = std::make_shared<Matcher<T, Tout, RelatedValueType>>(
				Matcher<T, Tout, RelatedValueType>::FirstMatch);
    	op->constructNFA(expr);
      auto iter = addPublisher<Matcher<T, Tout, RelatedValueType>, DataSource<T> >(op);
      return Pipe<Tout>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
    }

    /*---------------------------------- joins --------------------------------*/

    /**
     * @brief Creates an operator for joining two streams represented by pipes.
     *
     * Creates an operator implementing a symmetric hash join to join two streams.
     * In addition to the inherent key comparison of the hash join an additional
     * join predicate can be specified. Note, that the output tuple type is derived
     * from the two input types.
     *
     * @tparam T1
     *      the input tuple type (usually a TuplePtr) of the left stream.
     * @tparam T2
     *      the input tuple type (usually a TuplePtr) of the right stream.
     * @tparam KeyType
     *      the data type for representing keys (join values)
     * @param[in] otherPipe
     *      the pipe representing the right stream
     * @param[in] pred
     *      the join predicate which is applied in addition to the equi-join
     *      condition of the hash join
     * @return a new pipe
     */
 //   template <typename T2, typename KeyType = DefaultKeyType>
    template <typename KeyType = DefaultKeyType, typename T2>
    Pipe<typename SHJoin<T, T2, KeyType>::ResultElement> join(Pipe<T2>& otherPipe,
               typename SHJoin<T, T2, KeyType>::JoinPredicateFunc pred) throw (TopologyException) {
      assert(partitioningState == NoPartitioning);
      try {
        typedef std::function<KeyType(const T&)> LKeyExtractorFunc;
	      typedef std::function<KeyType(const T2&)> RKeyExtractorFunc;

        LKeyExtractorFunc fn1 = boost::any_cast<LKeyExtractorFunc>(keyExtractor);
	      RKeyExtractorFunc fn2 = boost::any_cast<RKeyExtractorFunc>(otherPipe.keyExtractor);

	      auto op = std::make_shared<SHJoin<T, T2, KeyType>>(fn1, fn2, pred);

	      auto pOp = castOperator<DataSource<T>>(getPublisher());
	      auto otherOp = castOperator<DataSource<T2>>(otherPipe.getPublisher());

        connectChannels(pOp->getOutputDataChannel(), op->getLeftInputDataChannel());
        connectChannels(pOp->getOutputPunctuationChannel(), op->getInputPunctuationChannel());

        connectChannels(otherOp->getOutputDataChannel(), op->getRightInputDataChannel());
        connectChannels(otherOp->getOutputPunctuationChannel(), op->getInputPunctuationChannel());

        auto iter = dataflow->addPublisher(op);
        return Pipe<typename SHJoin<T, T2, KeyType>::ResultElement>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      } catch (boost::bad_any_cast &e) {
        throw TopologyException("No KeyExtractor defined for join.");
      }
    }

    /*----------------------------- table operators ---------------------------*/

    /**
     * @brief Creates an operator storing stream tuples in the given table.
     *
     * Creates an operator which stores tuples from the input stream into
     * the given table and forwards them to its subscribers. Outdated tuples
     * are handled as deletes, non-outdated tuples either as insert (if the key
     * does not exist yet) or update (otherwise).
     *
     * @tparam T
     *    the input tuple type (usually a TuplePtr) for the operator which is also
     *    used as the record type of the table.
     * @tparam KeyType
     *    the data type representing keys in the table
     * @param[in] tbl
     *    a pointer to the table object where the tuples are stored
     * @param[in] autoCommit
     *    @c true if each tuple is handled within its own transaction context
     * @return a new pipe
     */
    template <typename KeyType = DefaultKeyType>
    Pipe<T> toTable(std::shared_ptr<Table<typename T::element_type, KeyType>> tbl, bool autoCommit = true) throw (TopologyException) {
      typedef std::function<KeyType(const T&)> KeyExtractorFunc;
      assert(partitioningState == NoPartitioning);

      try {
        KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);

        auto op = std::make_shared<ToTable<T, KeyType> >(tbl, keyFunc, autoCommit);
        auto iter = addPublisher<ToTable<T, KeyType>, DataSource<T> >(op);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      } catch (boost::bad_any_cast &e) {
        throw TopologyException("No KeyExtractor defined for toTable.");
      }
    }

    /**
     * @brief Create an operator for updating a given table with data from the
     *        incoming tuple.
     *
     * Create a Map operator that executes an update on the given table for
     * each incoming stream tuple. The update function is the following function
     *  @code
     *     RecordType updateFunc(const &T data, bool outdated, const RecordType& old)
     *  @endcode
     *  where @c data is the stream element, @outdated the flag for outdated tuples,
     *  and @old the old record from the table. This record should be updated
     *  and returned.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @tparam RecordType
     *    the record type of the table
     * @tparam KeyType
     *    the data type representing keys in the table
     * @param[in] tbl
     *    a pointer to the table object which is updated. This table has to be
     *    of type Table<RecordType::element_type, KeyType>
     * @param[in] updateFunc
     *    the update function which is executed for each stream tuple
     * @return a new pipe
     */
    template <typename RecordType, typename KeyType = DefaultKeyType>
    Pipe<T> updateTable(std::shared_ptr<Table<typename RecordType::element_type, KeyType>> tbl,
      std::function<bool(const T&, bool, const typename RecordType::element_type&)> updateFunc)
          throw (TopologyException) {
      typedef std::function<KeyType(const T&)> KeyExtractorFunc;
      assert(partitioningState == NoPartitioning);

      try {
        KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);
        return map<T, T>([=](auto tp, bool outdated) {
            KeyType key = keyFunc(tp);
            tbl->updateOrDeleteByKey(key, [=](const typename RecordType::element_type& old) -> bool {
                  return updateFunc(tp, outdated, old);
            });
            return tp;
        });
      } catch (boost::bad_any_cast &e) {
        throw TopologyException("No KeyExtractor defined for updateTable.");
      }
    }
    /*------------------------------ partitioning -----------------------------*/
    /**
     * @brief Create a PartitionBy operator.
     *
     * Crate a PartitionBy operator for partitioning the input stream on given partition id
     * which is derived using a user-defined function and forwarding the tuples of
     * each partition to a subquery. Subqueries are registered via their input channels
     * for each partition id.
     *
     * @tparam T
     *   the data stream element type consumed by PartitionBy
     *
     * @param pFun
     *        the function for deriving the partition id
  	 * @param numPartitions
     *        the number of partitions
     * @return a new pipe
     */
    Pipe<T> partitionBy(typename PartitionBy<T>::PartitionFunc pFun, unsigned int nPartitions)
      throw (TopologyException) {
        if (partitioningState != NoPartitioning)
          throw TopologyException("Cannot partition an already partitioned stream.");
      auto op = std::make_shared<PartitionBy<T>>(pFun, nPartitions);
      auto iter = addPublisher<PartitionBy<T>, DataSource<T> >(op);
      return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, FirstInPartitioning, nPartitions);
    }

    /**
     * @brief Create a Merge operator.
     *
     * Create a Merge operator which subscribes to multiple streams and combines all tuples
     * produced by these input stream into a single stream.
     *
     * @tparam T
     *   the data stream element type consumed by PartitionBy
     * @return a new pipe
     */
    Pipe<T> merge() throw (TopologyException) {
      if (partitioningState != NextInPartitioning)
        throw TopologyException("Nothing to merge in topology.");

      auto op = std::make_shared<Merge<T> >();
      for (auto iter = getPublishers(); iter != dataflow->publisherEnd(); iter++) {
        auto pOp = castOperator<DataSource<T>>(iter->get());
        CREATE_LINK(pOp, op);
      }
      auto iter = dataflow->addPublisher(op);
      // return Pipe(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);

      auto queue = std::make_shared<Queue<T> >();
      CREATE_LINK(op, queue);
      auto iter2 = dataflow->addPublisher(queue);

      return Pipe<T>(dataflow, iter2, keyExtractor, timestampExtractor, NoPartitioning, 0);
    }

    /*----------------------------- synchronization ---------------------------*/

    /**
     * @brief  Create a new barrier operator evaluating the given predicate
     * on each incoming tuple.
     *
     * @tparam
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param cVar a condition variable for signaling when the predicate
     *             shall be re-evaluated
     * @param mtx the mutex required to access the condition variable
     * @param f function pointer to a barrier predicate
     * @return a new pipe
     */
    Pipe<T> barrier(std::condition_variable& cVar, std::mutex& mtx, typename Barrier<T>::PredicateFunc f)
      throw (TopologyException) {
      if (partitioningState == NoPartitioning) {
        auto op = std::make_shared<Barrier<T> >(cVar, mtx, f);
        auto iter = addPublisher<Barrier<T>, DataSource<T> >(op);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
      else {
        std::vector<std::shared_ptr<Barrier<T>>> ops;
        for (auto i = 0u; i < numPartitions; i++) {
          ops.push_back(std::make_shared<Barrier<T> >(cVar, mtx, f));
        }
        auto iter = addPartitionedPublisher<Barrier<T>, T>(ops);
        return Pipe<T>(dataflow, iter, keyExtractor, timestampExtractor, partitioningState, numPartitions);
      }
    }
  };
}

#endif
