#ifndef TypeManager_hpp_
#define TypeManager_hpp_

#include <table/TableInfo.hpp>
#include <string>
#include <map>

#include "QueryCompileException.hpp"

namespace pfabric {

class TypeManager {
public:
  typedef std::pair<TableInfo, std::string> TypeInfo;
  typedef std::map<std::string, TypeInfo> TableTypeMap;
  typedef TableTypeMap::const_iterator TypeIterator;

  TypeManager() {}

  void registerType(const TableInfo& tInfo);
  std::string nameOfType(const TableInfo& tInfo) throw (QueryCompileException);

  TypeIterator begin() const { return mTypeTable.begin(); }
  TypeIterator end() const { return mTypeTable.end(); }

private:
  TableTypeMap mTypeTable;
};

}

#endif
