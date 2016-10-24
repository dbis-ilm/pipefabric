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
#include "cep/Matcher.hpp"
#include "qop/ZMQSink.hpp"

namespace pfabric {

  /**
   * @brief Pipe represents a sequence of operators applied to a data stream. Pipes are used
   *        mainly to construct a dataflow programatically.
   *
   * A Pipe is used to construct and represent a dataflow program. Pipes are constructed by
   * creating a new data source via the @c Topology class. Then, new operators can be added one
   * by one via methods of the @c Pipe class.
   */
  class Pipe {
    friend class Topology;
  private:
    enum PartitioningState {
        NoPartitioning,
        FirstInPartitioning,
        NextInPartitioning
    } partitioningState;

    /**
     * Typedef for pointer to BaseOp (any PipeFabric operator).
     */
    typedef std::shared_ptr<BaseOp> BaseOpPtr;
    typedef std::vector<BaseOpPtr> BaseOpList;
    typedef BaseOpList::iterator BaseOpIterator;

    BaseOpList publishers; // the list of all operators acting as publisher (source)
    BaseOpList sinks;     // the list of sink operators (which are not publishers)
    /// Note, we need boost::any values here because the functions pointers are typed (via templates)
    boost::any timestampExtractor; // a function pointer to a timestamp extractor function
    boost::any keyExtractor;            // a function pointer to a key extractor function
    unsigned int numPartitions;

    /**
     * @brief Constructor for Pipe.
     *
     * Creates a new pipe with the given operator @c op as initial publisher.
     *
     * @param[in] op
     *     an operator producing the tuple for the this pipeline
     */
    Pipe(BaseOpPtr op) : partitioningState(NoPartitioning), numPartitions(0) {
      publishers.push_back(op);
    }

    /**
     * @brief Returns the operator at the end of the publisher list.
     *
     * Returns the operator which acts as the publisher for the next
     * added operator.
     *
     * @return
     *    the last operator in the publisher list
     */
    BaseOpPtr getPublisher() { return publishers.back(); }

    BaseOpIterator getPublishers() {
      return publishers.end() - numPartitions;
    }

    template<typename Publisher, typename SourceType>
    void addPublisher(std::shared_ptr<Publisher> op) {
      auto pOp = dynamic_cast<SourceType*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
    }

    template<typename Publisher, typename StreamElement>
    void addPartitionedPublisher(std::vector<std::shared_ptr<Publisher>>& opList) {
      BOOST_ASSERT_MSG(partitioningState != NoPartitioning, "Missing PartitionBy operator in topology.");
      if (partitioningState == FirstInPartitioning) {
        auto partition = dynamic_cast<PartitionBy<StreamElement> *>(getPublisher().get());
        BOOST_ASSERT_MSG(partition != nullptr,
          "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
        for (int i = 0; i < opList.size(); i++) {
          auto& op = opList[i];
          partition->connectChannelsForPartition(i,
            op->getInputDataChannel(),
            op->getInputPunctuationChannel());
        }
        partitioningState = NextInPartitioning;
      }
      else {
        auto iter = getPublishers();
        for (int i = 0; i < opList.size() && iter != publishers.end(); i++) {
          auto p = iter->get();
          auto pOp = dynamic_cast<DataSource<StreamElement> *>(p);
          BOOST_ASSERT_MSG(pOp != nullptr,
            "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
          CREATE_LINK(pOp, opList[i]);
          iter++;
        }
      }
      for (auto& op : opList)
        publishers.push_back(op);
    }

  public:
    /**
     * @brief Destructor of the pipe.
     *
     * Destroys the pipe and removes all publishers.
     */
    ~Pipe() { publishers.clear(); }

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
     * @return a reference to the pipe
     */
    template <typename T, typename KeyType = DefaultKeyType>
    Pipe& keyBy(std::function<KeyType(const T&)> func) {
      keyExtractor = func;
      return *this;
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
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& assignTimestamps(typename Window<T>::TimestampExtractorFunc func) {
      timestampExtractor = func;
      return *this;
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
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& slidingWindow(const WindowParams::WinType& wt,
                        const unsigned int sz,
                        const unsigned int ei = 0) {
      typedef typename Window<T>::TimestampExtractorFunc ExtractorFunc;
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
        addPublisher<SlidingWindow<T>, DataSource<T>>(op);
      }
      catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No TimestampExtractor defined for SlidingWindow.");
      }
      return *this;
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
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& tumblingWindow(const WindowParams::WinType& wt,
                         const unsigned int sz) {
      typedef typename Window<T>::TimestampExtractorFunc ExtractorFunc;
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
        addPublisher<TumblingWindow<T>, DataSource<T>>(op);
      }
      catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No TimestampExtractor defined for TumblingWindow.");
      }
      return *this;
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
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& print(std::ostream& os = std::cout,
                typename ConsoleWriter<T>::FormatterFunc ffun = ConsoleWriter<T>::defaultFormatter) {
      auto op = std::make_shared<ConsoleWriter<T> >(os, ffun);
      auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      // we don't save the operator in publishers because ConsoleWriter
      // cannot act as Publisher
      sinks.push_back(op);
      return *this;
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
     * @return a reference to the pipe
     */
   template <typename T>
    Pipe& saveToFile(const std::string& fname,
                     typename FileWriter<T>::FormatterFunc ffun = ConsoleWriter<T>::defaultFormatter) {
      auto op = std::make_shared<FileWriter<T> >(fname, ffun);
      auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      // we don't save the operator in publishers because FileWriter
      // cannot act as Publisher
      sinks.push_back(op);
      return *this;
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
     * @return a reference to the pipe
     */
   template <typename T>
   Pipe& sendZMQ(const std::string& path, ZMQParams::SinkType stype = ZMQParams::PublisherSink,
       ZMQParams::EncodingMode mode = ZMQParams::BinaryMode) {
      auto op = std::make_shared<ZMQSink<T> >(path, stype, mode);
      auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      // we don't save the operator in publishers because ZMQSink
      // cannot act as Publisher
      sinks.push_back(op);
      return *this;
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
     * @return a reference to the pipe
     */
    template <class T>
    Pipe& extract(char sep) {
      auto op = std::make_shared<TupleExtractor<T> >(sep);
      addPublisher<TupleExtractor<T>, DataSource<TStringPtr> >(op);
      return *this;
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
     * @return a reference to the pipe
     */
    template <class T>
    Pipe& extractJson(const std::initializer_list<std::string>& keys) {
      std::vector<std::string> keyList(keys);
      auto op = std::make_shared<JsonExtractor<T> >(keyList);
      addPublisher<JsonExtractor<T>, DataSource<TStringPtr> >(op);
      return *this;
    }

    /**
     * @brief
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @return a reference to the pipe
     */
    template <class T>
    Pipe& deserialize() {
      auto op = std::make_shared<TupleDeserializer<T> >();
      addPublisher<TupleDeserializer<T>, DataSource<TBufPtr> >(op);
      return *this;
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
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& where(typename Where<T>::PredicateFunc func) {
      if (partitioningState == NoPartitioning) {
        auto op = std::make_shared<Where<T> >(func);
        addPublisher<Where<T>, DataSource<T> >(op);
      }
      else {
        std::vector<std::shared_ptr<Where<T>>> ops;
        for (int i = 0; i < numPartitions; i++) {
          ops.push_back(std::make_shared<Where<T> >(func));
        }
        addPartitionedPublisher<Where<T>, T>(ops);
      }
      return *this;
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
      * @return a reference to the pipe
      */
     template <typename T>
     Pipe& notify(typename Notify<T>::CallbackFunc func) {
       auto op = std::make_shared<Notify<T> >(func);
       addPublisher<Notify<T>, DataSource<T> >(op);
       return *this;
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
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& queue() {
      auto op = std::make_shared<Queue<T> >();
      addPublisher<Queue<T>, DataSource<T> >(op);
      return *this;
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
     * @return a reference to the pipe
     */
    template <typename Tin, typename Tout>
    Pipe& map(typename Map<Tin, Tout>::MapFunc func) {
      if (partitioningState == NoPartitioning) {
        auto op = std::make_shared<Map<Tin, Tout> >(func);
        addPublisher<Map<Tin, Tout>, DataSource<Tin> >(op);
      }
      else {
        std::vector<std::shared_ptr<Map<Tin, Tout>>> ops;
        for (int i = 0; i < numPartitions; i++) {
          ops.push_back(std::make_shared<Map<Tin, Tout>>(func));
        }
        addPartitionedPublisher<Map<Tin, Tout>, Tin>(ops);
      }
      return *this;
    }

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
     * @return a reference to the pipe
     */
    template <typename Tin, typename Tout, typename AggrState>
    Pipe& aggregate(AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) {
      return aggregate<Tin, Tout, AggrState>(AggrState::finalize, AggrState::iterate, tType, tInterval);
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
      * @return a reference to the pipe
      */
      template <typename Tin, typename Tout, typename AggrState>
      Pipe& aggregate(typename Aggregation<Tin, Tout, AggrState>::FinalFunc finalFun,
                     typename Aggregation<Tin, Tout, AggrState>::IterateFunc iterFun,
                     AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) {
       auto op = std::make_shared<Aggregation<Tin, Tout, AggrState> >(std::make_shared<AggrState>(),
             finalFun, iterFun, tType, tInterval);
       addPublisher<Aggregation<Tin, Tout, AggrState>, DataSource<Tin> >(op);
       return *this;
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
      * @return a reference to the pipe
      */
     template <typename Tin, typename Tout, typename AggrState, typename KeyType = DefaultKeyType>
     Pipe& groupBy(AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) {
       return groupBy<Tin, Tout, AggrState, KeyType>(AggrState::finalize, AggrState::iterate, tType, tInterval);
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
     * @return a reference to the pipe
     */
    template <typename Tin, typename Tout, typename AggrState, typename KeyType = DefaultKeyType>
    Pipe& groupBy(std::shared_ptr<AggrState> aggrStatePtr,
                    typename GroupedAggregation<Tin, Tout, AggrState, KeyType>::FinalFunc finalFun,
                    typename GroupedAggregation<Tin, Tout, AggrState, KeyType>::IterateFunc iterFun,
                    AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) {
      try {
        typedef std::function<KeyType(const Tin&)> KeyExtractorFunc;
        KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);

        auto op =
          std::make_shared<GroupedAggregation<Tin, Tout, AggrState, KeyType> >(aggrStatePtr,
              keyFunc, finalFun, iterFun, tType, tInterval);
        addPublisher<GroupedAggregation<Tin, Tout, AggrState, KeyType>, DataSource<Tin> >(op);
      } catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No KeyExtractor defined for groupBy.");
      }
      return *this;
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
     * @param[in] aggrStatePtr
     *      an instance of our working NFA
     * @return a reference to the pipe
     * TODO: make better
     */
    template <typename Tin, typename Tout, typename RelatedValueType>
    Pipe& matchByNFA(typename NFAController<Tin, Tout, RelatedValueType>::NFAControllerPtr nfa) {
    	auto op = std::make_shared<Matcher<Tin, Tout, RelatedValueType>>(
				Matcher<Tin, Tout, RelatedValueType>::FirstMatch);
    	op->setNFAController(nfa);
      addPublisher<Matcher<Tin, Tout, RelatedValueType>, DataSource<Tin> >(op);
    	return *this;
    }
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
     * @return a reference to the pipe
     */
    template <typename T1, typename T2, typename KeyType = DefaultKeyType>
    Pipe& join(Pipe& otherPipe,
               typename SHJoin<T1, T2, KeyType>::JoinPredicateFunc pred) {
      try {
        typedef std::function<KeyType(const T1&)> LKeyExtractorFunc;
	      typedef std::function<KeyType(const T2&)> RKeyExtractorFunc;

        LKeyExtractorFunc fn1 = boost::any_cast<LKeyExtractorFunc>(keyExtractor);
	      RKeyExtractorFunc fn2 = boost::any_cast<RKeyExtractorFunc>(otherPipe.keyExtractor);

	      auto op = std::make_shared<SHJoin<T1, T2, KeyType>>(fn1, fn2, pred);

	      auto pOp = dynamic_cast<DataSource<T1>*>(getPublisher().get());
	      BOOST_ASSERT_MSG(pOp != nullptr,
          "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");

	      auto otherOp = dynamic_cast<DataSource<T2>*>(otherPipe.getPublisher().get());
	      BOOST_ASSERT_MSG(otherOp != nullptr,
          "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");

        connectChannels(pOp->getOutputDataChannel(), op->getLeftInputDataChannel());
        connectChannels(pOp->getOutputPunctuationChannel(), op->getInputPunctuationChannel());

        connectChannels(otherOp->getOutputDataChannel(), op->getRightInputDataChannel());
        connectChannels(otherOp->getOutputPunctuationChannel(), op->getInputPunctuationChannel());

	      publishers.push_back(op);
      } catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No KeyExtractor defined for SHJoin.");
      }
      return *this;
    }

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
     * @return a reference to the pipe
     */
    template <typename T, typename KeyType = DefaultKeyType>
    Pipe& toTable(std::shared_ptr<Table<T, KeyType>> tbl, bool autoCommit = true) {
      typedef std::function<KeyType(const T&)> KeyExtractorFunc;

      try {
        KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);

        auto op = std::make_shared<ToTable<T, KeyType> >(tbl, keyFunc, autoCommit);
        addPublisher<ToTable<T, KeyType>, DataSource<T> >(op);
      } catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No KeyExtractor defined for ToTable.");
      }

      return *this;
    }

    template <typename T>
    Pipe& partitionBy(typename PartitionBy<T>::PartitionFunc pFun, unsigned int nPartitions) {
      auto op = std::make_shared<PartitionBy<T>>(pFun, nPartitions);
      addPublisher<PartitionBy<T>, DataSource<T> >(op);
      partitioningState = FirstInPartitioning;
      numPartitions = nPartitions;
      return *this;
    }

    template <typename T>
    Pipe& merge() {
      auto op = std::make_shared<Merge<T> >();
      std::cout << "partitioningState = " << partitioningState << std::endl;
      BOOST_ASSERT_MSG(partitioningState == NextInPartitioning, "Nothing to merge in topology.");
      for (auto iter = getPublishers(); iter != publishers.end(); iter++) {
        auto pOp = dynamic_cast<DataSource<T> *>(iter->get());
        BOOST_ASSERT_MSG(pOp != nullptr,
          "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
        CREATE_LINK(pOp, op);
      }
      publishers.push_back(op);
      partitioningState = NoPartitioning;
      return *this;
    }
  };
}

#endif
