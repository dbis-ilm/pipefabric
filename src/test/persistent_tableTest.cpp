#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do
// this in one cpp file

#include "catch.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

#include "table/TableInfo.hpp"
#include "core/Tuple.hpp"
#include "core/serialize.hpp"
#include "pfabric.hpp"

#include "fmt/format.h"

using namespace pfabric;
using nvml::obj::pool;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::make_persistent;
using nvml::obj::transaction;

typedef pfabric::Tuple<int, int, string, double> MyTuple;
typedef pfabric::nvm::persistent_table<MyTuple, uint64_t> pTable_type;

struct root {
    persistent_ptr<pTable_type> pTable;
};

TEST_CASE("Testing storing tuples in persistent_table", "[persistent_table]") {
  pool<root> pop;
  const std::string path = "/tmp/testdb.db";
  std::remove(path.c_str());

  if (access(path.c_str(), F_OK) != 0) {
    pop = pool<root>::create(path, LAYOUT);
  } else {
    pop = pool<root>::open(path, LAYOUT);
  }

  auto q = pop.get_root();
  if (!q->pTable) {
    auto tInfo = TableInfo("MyTable", {
                           ColumnInfo("a", ColumnInfo::Int_Type),
                           ColumnInfo("b", ColumnInfo::Int_Type),
                           ColumnInfo("c", ColumnInfo::String_Type),
                           ColumnInfo("d", ColumnInfo::Double_Type)
    });
    transaction::exec_tx(pop,
        [&] {q->pTable = make_persistent<pTable_type>(tInfo);});
  } else {
    std::cerr << "WARNING: Table already exists" << std::endl;
  }

  for (unsigned int i = 0; i < 10; i++) {
    auto tup = MyTuple(i + 1,
                       (i + 1) * 100,
                       fmt::format("String #{0}", i),
                       i * 12.345);
    q->pTable->insert(tup);
  }
  //q->pTable->print(true);

  /* Clean up */
  transaction::exec_tx(pop, [&] {delete_persistent<pTable_type>(q->pTable);});
  pop.close();
}
