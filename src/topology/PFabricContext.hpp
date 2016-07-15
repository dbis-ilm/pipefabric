#ifndef PFabricContext_hpp_
#define PFabricContext_hpp_

#include <string>
#include <map>

#include "core/PFabricTypes.hpp"
#include "table/Table.hpp"
#include "topology/Topology.hpp"

namespace pfabric {

class PFabricContext {
public:
  typedef std::shared_ptr<Topology> TopologyPtr;

  PFabricContext();
  ~PFabricContext();

  TopologyPtr createTopology();

  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> createTable(const std::string& tblName) throw (TableException) {
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end())
      throw TableException();

    auto tbl = std::make_shared<Table<RecordType, KeyType>>();
    mTableSet[tblName] = tbl;
    return tbl;
  }

  template <typename RecordType, typename KeyType = DefaultKeyType>
  std::shared_ptr<Table<RecordType, KeyType>> getTable(const std::string& tblName) {
    auto it = mTableSet.find(tblName);
    if (it != mTableSet.end()) {
      return std::static_pointer_cast<Table<RecordType, KeyType>>(it->second);
    }
    else
      return std::shared_ptr<Table<RecordType, KeyType>>();
  }

private:
  typedef std::shared_ptr<BaseTable> BaseTablePtr;

  std::map<std::string, BaseTablePtr> mTableSet;

};

}

#endif
