#ifndef PlanCache_hpp_
#define PlanCache_hpp_

#include <map>
#include <string>
#include <stdexcept>
	
#include "TopologyBuilder.hpp"

namespace pfabric {

  struct CacheEntry {
    CacheEntry(TopologyBuilderPtr b, PFabricContext::TopologyPtr t, const std::string& l) :
      builder(b), topology(t), libraryName(l) {}

    TopologyBuilderPtr builder;
    PFabricContext::TopologyPtr topology;
    std::string libraryName;
  };

  class PlanCache {
    public:
      PlanCache() {}

      void addToCache(const std::string& queryString, const CacheEntry& entry);
      const CacheEntry& findPlanForQuery(const std::string& queryString) throw (std::out_of_range);

     private:
        std::map<std::string, CacheEntry> planCache;
   };
}

#endif

