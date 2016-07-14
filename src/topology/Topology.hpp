#ifndef Topology_hpp_
#define Topology_hpp_

#include <string>
#include <list>
#include <vector>

#include "core/Tuple.hpp"

#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "qop/RESTSource.hpp"
#include "qop/ZMQSource.hpp"
#include "qop/ToTable.hpp"
#include "qop/FromTable.hpp"

#include "topology/Pipe.hpp"

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
   * Topology t;
   * auto s = t.newStreamFromFile("file.csv")
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
   * t.start();
   * @endcode
   */
  class Topology {
  private:
    /// the signature of a startup function
    typedef std::function<unsigned long()> StartupFunc;

    std::list<Pipe*> pipes;               //< the list of pipes created for this topology
    std::vector<StartupFunc> startupList; //< the list of functions to be called for startup

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

    /**
     * @brief Invokes the start functions asynchronously.
     */
    void startAsync();

  public:
    /**
     * @brief Constructs a new empty topology.
     */
    Topology() {}

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

    /**
     * @brief Creates a pipe from a TextFileSource as input.
     *
     * Creates a new pipe for reading tuples (containing only a
     * string field representing a line of the file) via a
     * TextFileSource.
     *
     * @param[in] fname
     *    the name of the file from which the tuples are read.
     * @return
     *    a new pipe where TextFileSource acts as a producer.
     */
    Pipe& newStreamFromFile(const std::string& fname);

    Pipe& newStreamFromREST(unsigned int port,
      const std::string& path,
      RESTSource::RESTMethod method,
      unsigned short numThreads = 1);

    Pipe& newStreamFromZMQ(const std::string& path,
      ZMQParams::EncodingMode encoding = ZMQParams::AsciiMode,
      ZMQParams::SourceType stype = ZMQParams::SubscriberSource);

    template<typename T, typename KeyType = DefaultKeyType>
    Pipe& newStreamFromTable(std::shared_ptr<Table<T, KeyType>> tbl,
      typename Table<T, KeyType>::NotificationMode mode = Table<T, KeyType>::Immediate) {
      auto op = std::make_shared<FromTable<T, KeyType>>(tbl, mode);
      auto s = new Pipe(op);
      pipes.push_back(s);
      return *s;
    }
  };

}

#endif
