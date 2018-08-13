#include "Workload.h"

using namespace pfabric;

void generateWorkload(const std::string &name) {
  Workload<ResultPtr::element_type> wl;
  std::ofstream workload_file;

  workload_file.open(name);

  for (auto t = 2u; t <= workloadNumTxs+1; ++t) {
    for(auto k = 0u; k < txSize; ++k) {
      auto key = dis(gen);
      wl.addEntry(t, {key, key * 100, t * 1.23});
    }
  }

  //wl.shuffle();

  wl.serialize(workload_file);
  workload_file.close();
}

int main() {
  generateWorkload("wl_writes.csv");
}
