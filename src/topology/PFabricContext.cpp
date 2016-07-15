#include "PFabricContext.hpp"

using namespace pfabric;

PFabricContext::PFabricContext() {}
PFabricContext::~PFabricContext() {}

PFabricContext::TopologyPtr PFabricContext::createTopology() {
  return std::make_shared<Topology>();
}
