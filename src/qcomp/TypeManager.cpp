#include "TypeManager.hpp"
#include "QueryCompileException.hpp"
#include "UniqueNameGenerator.hpp"

using namespace pfabric;

void TypeManager::registerType(const TableInfo& tInfo) {
  auto sig = tInfo.typeSignature();
  if (mTypeTable.count(sig) == 0) {
    std::string typeName = UniqueNameGenerator::instance()->uniqueName("Tuple") + "_Type_";
    mTypeTable[sig] = std::make_pair(tInfo, typeName);
  }
}

std::string TypeManager::nameOfType(const TableInfo& tInfo) throw (QueryCompileException) {
  auto iter = mTypeTable.find(tInfo.typeSignature());
  if (iter == mTypeTable.end()) 
    throw QueryCompileException("unkown type");

  auto typeInfo = iter->second;
  return typeInfo.second;
}

