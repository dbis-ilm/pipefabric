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
#include "qop/Queue.hpp"
#include "qop/Map.hpp"
#include "qop/TupleExtractor.hpp"
#include "qop/TupleDeserializer.hpp"
#include "qop/ConsoleWriter.hpp"
#include "qop/FileWriter.hpp"
#include "qop/SlidingWindow.hpp"
#include "qop/TumblingWindow.hpp"
#include "qop/Aggregation.hpp"
#include "qop/GroupedAggregation.hpp"
#include "qop/SHJoin.hpp"
#include "qop/ToTable.hpp"

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
    typedef std::shared_ptr<BaseOp> BaseOpPtr;
    std::list<BaseOpPtr> publishers; // the list of all operators acting as publisher (source)
    std::list<BaseOpPtr> sinks;     // the list of sink operators (which are not publishers)
    /// Note, we need boost::any values here because the functions pointers are typed (via templates)
    boost::any timestampExtractor; // a function pointer to a timestamp extractor function
    boost::any keyExtractor;            // a function pointer to a key extractor function

    /**
     * @brief Constructor for Pipe.
     *
     * Creates a new pipe with the given operator @c op as initial publisher.
     *
     * @param[in] op
     *     an operator producing the tuple for the this pipeline
     */
    Pipe(std::shared_ptr<BaseOp> op) {
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
    BaseOpPtr getPublisher() const { return publishers.back(); }

  public:
    /**
     * @brief Destructor of the pipe.
     */
    ~Pipe() { publishers.clear(); }

    /**
     * @brief Defines the key extractor function for all subsequent operators.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
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
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] wt
     *      the type of the window (row or range)
     * @param[in] sz
     *      the window size (in number of tuples for row window or in milliseconds for range
     *      windows)
     * @param[in] sd
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& slidingWindow(const WindowParams::WinType& wt, const unsigned int sz,
                        const unsigned int sd = 0) {
      typedef typename Window<T>::TimestampExtractorFunc ExtractorFunc;

      try {
        ExtractorFunc fn;
        std::shared_ptr<SlidingWindow<T> > op;

        if (wt == WindowParams::RangeWindow) {
          fn = boost::any_cast<ExtractorFunc>(timestampExtractor);
          op = std::make_shared<SlidingWindow<T> >(fn, wt, sz, sd);
        }
        else
          op =  std::make_shared<SlidingWindow<T> >(wt, sz, sd);

        auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
        BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
        CREATE_LINK(pOp, op);
        publishers.push_back(op);
      }
      catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No TimestampExtractor defined for SlidingWindow.");
      }
      return *this;
    }

    /**
     * @brief Creates a tumbling window operator as the next operator on the pipe.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] wt
     *      the type of the window (row or range)
     * @param[in] sz
     *      the window size (in number of tuples for row window or in milliseconds for range
     *      windows)
     * @param[in] sd
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& tumblingWindow(const WindowParams::WinType& wt, const unsigned int sz) {
      typedef typename Window<T>::TimestampExtractorFunc ExtractorFunc;
      try {
        ExtractorFunc fn;
        std::shared_ptr<TumblingWindow<T> > op;

        if (wt == WindowParams::RangeWindow) {
          fn = boost::any_cast<ExtractorFunc>(timestampExtractor);
          op = std::make_shared<TumblingWindow<T> >(fn, wt, sz);
        }
        else
          op = std::make_shared<TumblingWindow<T> >(wt, sz);

        auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
        BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
        CREATE_LINK(pOp, op);
        publishers.push_back(op);
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
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] ffun
     * @param[in] os
     * @return a reference to the pipe
     */
    template <typename T>
    Pipe& print(std::ostream& os = std::cout, typename ConsoleWriter<T>::FormatterFunc ffun = ConsoleWriter<T>::defaultFormatter) {
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
     * @brief Creates an operator for saving tuples to a file (FileWriter)
     *        as the next operator on the pipe.
     *
     * @tparam T
     *      the input tuple type (usually a TuplePtr) for the operator.
     * @param[in] ffun
     * @param[in] fname
     * @return a reference to the pipe
     */
   template <typename T>
    Pipe& saveToFile(const std::string& fname, typename FileWriter<T>::FormatterFunc ffun = ConsoleWriter<T>::defaultFormatter) {
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
     * @brief Creates an operator for extracting typed fields from a simple string tuple
     *        as the next operator on the pipe.
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
      auto pOp = dynamic_cast<DataSource<TStringPtr>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
      return *this;
    }

    template <class T>
    Pipe& deserialize() {
      auto op = std::make_shared<TupleDeserializer<T> >();
      auto pOp = dynamic_cast<DataSource<TBufPtr>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
      return *this;
    }
   /**
     * @brief Creates a filter operator which forwards only tuples satisfying the given filter predicate
     *        as the next operator on the pipe.
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
      auto op = std::make_shared<Where<T> >(func);
      auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
      return *this;
    }

    template <typename T>
    Pipe& queue() {
      auto op = std::make_shared<Queue<T> >();
      auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
      return *this;
    }

    /**
     * @brief Creates a map operator which applies a mapping (projection) function to each tuples
     *        as the next operator on the pipe.
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
      auto op = std::make_shared<Map<Tin, Tout> >(func);
      auto pOp = dynamic_cast<DataSource<Tin>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
      return *this;
    }

    template <typename Tin, typename Tout, typename AggrState>
    Pipe& aggregate(std::shared_ptr<AggrState> aggrStatePtr,
                    typename Aggregation<Tin, Tout, AggrState>::FinalFunc finalFun,
                    typename Aggregation<Tin, Tout, AggrState>::IterateFunc iterFun,
                    AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) {
      auto op = std::make_shared<Aggregation<Tin, Tout, AggrState> >(aggrStatePtr, finalFun, iterFun, tType, tInterval);
      auto pOp = dynamic_cast<DataSource<Tin>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr,
        "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
      return *this;
    }

    template <typename Tin, typename Tout, typename AggrState, typename KeyType = DefaultKeyType>
    Pipe& groupBy(std::shared_ptr<AggrState> aggrStatePtr,
                    typename GroupedAggregation<Tin, Tout, AggrState, KeyType>::FinalFunc finalFun,
                    typename GroupedAggregation<Tin, Tout, AggrState, KeyType>::IterateFunc iterFun,
                    AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) {
      try {
      typedef std::function<KeyType(const Tin&)> KeyExtractorFunc;
      KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);

      auto op = std::make_shared<GroupedAggregation<Tin, Tout, AggrState, KeyType> >(aggrStatePtr, keyFunc, finalFun, iterFun, tType, tInterval);
      auto pOp = dynamic_cast<DataSource<Tin>*>(getPublisher().get());
      BOOST_ASSERT_MSG(pOp != nullptr, "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
      CREATE_LINK(pOp, op);
      publishers.push_back(op);
    } catch (boost::bad_any_cast &e) {
      BOOST_ASSERT_MSG(false, "No KeyExtractor defined for groupBy.");
    }
      return *this;
    }

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

    template <typename T, typename KeyType = DefaultKeyType>
    Pipe& toTable(std::shared_ptr<Table<T, KeyType>> tbl, bool autoCommit = true) {
      typedef std::function<KeyType(const T&)> KeyExtractorFunc;

      try {
        KeyExtractorFunc keyFunc = boost::any_cast<KeyExtractorFunc>(keyExtractor);

        auto op = std::make_shared<ToTable<T, KeyType> >(tbl, keyFunc, autoCommit);
        auto pOp = dynamic_cast<DataSource<T>*>(getPublisher().get());
        BOOST_ASSERT_MSG(pOp != nullptr,
          "Cannot obtain DataSource from pipe probably due to incompatible tuple types.");
        CREATE_LINK(pOp, op);
        publishers.push_back(op);
      } catch (boost::bad_any_cast &e) {
        BOOST_ASSERT_MSG(false, "No KeyExtractor defined for ToTable.");
      }

      return *this;
    }
  };
}

#endif
