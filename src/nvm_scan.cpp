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

  start = std::chrono::high_resolution_clock::now();

  struct root {
    persistent_ptr<PTableType> pTable;
  };

  pool<root> pop;

  const std::string path = "/mnt/pmem/tests/testdb.db";

  start = std::chrono::high_resolution_clock::now();

  if (access(path.c_str(), F_OK) != 0) {
    std::cerr << "ERROR: Table not found" << std::endl;
    return 1;
  } else {
    pop = pool<root>::open(path, LAYOUT);
  }

  auto pTable = pop.get_root()->pTable;

  /* RangeScan using Block iterator */

    auto iter = pTable->rangeScan(ColumnRangeMap({{0, std::make_pair<int, int>(1000, 2000)}/*,
                                                {2, std::make_pair<std::string, std::string>("String #1", "String #9")}*/}));
  end = std::chrono::high_resolution_clock::now();
  std::cout << "Init time in µs: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << '\n';
  start = std::chrono::high_resolution_clock::now();
    for (const auto &tp: iter) {
      tp;
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "RangeScan in µs: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
              << '\n';


  /* Scan via PTuple iterator */
  {
    start = std::chrono::high_resolution_clock::now();
    auto eIter = pTable->end();
    auto iter = pTable->select(
      [](const PTuple<MyTuple, int> &tp) {
        return (tp.get<0>() >= 800) && (tp.get<0>() <= 900);
      });
    for (; iter != eIter; iter++) {
      (*iter).get<0>();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "PTupleScan in µs: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
              << '\n';
  }

  pop.close();
}

