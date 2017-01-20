#include "PlanCache.hpp"

using namespace pfabric;


void PlanCache::addToCache(const std::string& queryString, const CacheEntry& entry) {
  planCache.insert({ queryString, entry });
}

const CacheEntry& PlanCache::findPlanForQuery(const std::string& queryString) throw (std::out_of_range) {
  auto iter = planCache.find(queryString);
  if (iter == planCache.end())
    throw std::out_of_range("query not found in cache");
  else
    return iter->second;
}
