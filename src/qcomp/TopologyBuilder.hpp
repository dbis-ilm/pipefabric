#ifndef TopologyBuilder_hpp_
#define TopologyBuilder_hpp_

#include <memory>
#include "pfabric.hpp"

namespace pfabric {

class TopologyBuilder {
public:
  virtual PFabricContext::TopologyPtr create(pfabric::PFabricContext& ctx) = 0;
  virtual ~TopologyBuilder() {
    std::cout << "TopologyBuilder::~TopologyBuilder" << std::endl;
  }
};

typedef boost::shared_ptr<TopologyBuilder> TopologyBuilderPtr;

 
}

#define BUILDER_CLASS(ClassName) \
struct ClassName : public TopologyBuilder { \
  virtual std::shared_ptr<Topology> create(PFabricContext& ctx); \
\
  PFabricContext::TopologyPtr topology; \
};

#endif
