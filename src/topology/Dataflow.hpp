#ifndef Dataflow_hpp_
#define Dataflow_hpp_

#include <string>
#include <vector>

#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"

namespace pfabric {

  struct Dataflow {
    /**
     * Typedef for pointer to BaseOp (any PipeFabric operator).
     */
    typedef std::shared_ptr<BaseOp> BaseOpPtr;
    typedef std::vector<BaseOpPtr> BaseOpList;
    typedef BaseOpList::iterator BaseOpIterator;

    BaseOpList publishers; //< the list of all operators acting as publisher (source)
    BaseOpList sinks;     //< the list of sink operators (which are not publishers)

    BaseOpIterator addPublisher(BaseOpPtr op) {
      publishers.push_back(op);
      return publishers.end() - 1;
    }

    void addSink(BaseOpPtr op) { sinks.push_back(op); }

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

    BaseOpIterator getPublishers(unsigned int num) {
      return publishers.end() - num;
    }

    std::size_t size() const { return publishers.size(); }
};

typedef std::shared_ptr<Dataflow> DataflowPtr;
}

#endif
