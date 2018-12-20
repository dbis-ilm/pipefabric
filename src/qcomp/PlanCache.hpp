/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

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
      const CacheEntry& findPlanForQuery(const std::string& queryString);

     private:
        std::map<std::string, CacheEntry> planCache;
   };
}

#endif

