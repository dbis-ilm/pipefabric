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
