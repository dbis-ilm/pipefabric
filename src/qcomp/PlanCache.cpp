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

#include "PlanCache.hpp"

using namespace pfabric;


void PlanCache::addToCache(const std::string& queryString, const CacheEntry& entry) {
  planCache.insert({ queryString, entry });
}

const CacheEntry& PlanCache::findPlanForQuery(const std::string& queryString) {
  auto iter = planCache.find(queryString);
  if (iter == planCache.end())
    throw std::out_of_range("query not found in cache");
  else
    return iter->second;
}
