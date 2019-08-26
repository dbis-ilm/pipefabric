#include "common.h"
#include "Workload.h"

using namespace pfabric;

int main() {
  const std::string fname = zipf? "wl_writes_zipf.csv" : "wl_writes_uni.csv";

	generateWorkload<zipf>(1.05, "test1.05.csv");
}
