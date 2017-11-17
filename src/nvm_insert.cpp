#include "pfabric.hpp"

using namespace pfabric::nvm;
using nvml::obj::make_persistent;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::pool;
using nvml::obj::transaction;

using MyTuple = pfabric::Tuple<int, int, string, double>;
using PTableType = pfabric::PTable<MyTuple, int>;

int main() {
  std::chrono::high_resolution_clock::time_point start, end;
  std::vector<typename std::chrono::duration<int64_t, micro>::rep> measures;

  struct root {
    persistent_ptr<PTableType> pTable;
  };

  pool<root> pop;

  const std::string path = "/mnt/pmem/tests/testdb.db";

  std::remove(path.c_str());

  if (access(path.c_str(), F_OK) != 0) {
    pop = pool<root>::create(path, LAYOUT, 1 << 30);
    transaction::exec_tx(pop, [&] {
      using namespace pfabric;
      auto tInfo = TableInfo("MyTable", {
        ColumnInfo("a", ColumnInfo::Int_Type),
        ColumnInfo("b", ColumnInfo::Int_Type),
        ColumnInfo("c", ColumnInfo::String_Type),
        ColumnInfo("d", ColumnInfo::Double_Type)
      });
      pop.get_root()->pTable = make_persistent<PTableType>(tInfo, ColumnIntMap({{0, 10}/*, {3, 10}*/}));
    });
  } else {
    std::cerr << "WARNING: Table already exists" << std::endl;
    pop = pool<root>::open(path, LAYOUT);
  }

  auto pTable = pop.get_root()->pTable;

  for (unsigned int i = 0; i < 1000000; i++) {
    auto tup = MyTuple(i + 1,
                       (i + 1) * 100,
                       fmt::format("String #{0}", i),
                       i * 12.345);
    start = std::chrono::high_resolution_clock::now();
    pTable->insert(i + 1, tup);
    end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    measures.push_back(diff);
  }

  auto avg = std::accumulate(measures.begin(), measures.end(), 0) / measures.size();
  auto minmax = std::minmax_element(std::begin(measures), std::end(measures));
  std::cout << "\nInsert Statistics in µs: "
            << "\n\tAvg: \t" << avg
            << "\n\tMin: \t" << *minmax.first
            << "\n\tMax: \t" << *minmax.second << '\n';

  pop.close();
}

