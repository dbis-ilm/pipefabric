#include "table/StateContext.hpp"
#include "Workload.h"
#include <cmath>
#include <random>

using namespace pfabric;

void generateWorkload(const bool zipf, const double zipfTheta, const std::string &name) {
	Workload<ResultPtr::element_type> wl;
	std::ofstream workload_file;

	workload_file.open(name);

  if(zipf){
  	ZipfianGenerator zipfGen{0, keyRange-1, zipfTheta};
    std::cout << "Using Zipf with theta = " << zipfTheta << '\n';
	  for (auto t = 1u; t < workloadNumTxs+1; ++t) {
		  for(auto k = 0u; k < txSize; ++k) {
			  auto key = zipfGen.nextValue();
			  wl.addEntry(t, {key, key * 100, t * 1.23});
		  }
	  }
  } else {
    
    std::cout << "Using Uni with maximum value = " << uniMax << '\n';
	  for (auto t = 1u; t < workloadNumTxs+1; ++t) {
		  for(auto k = 0u; k < txSize; ++k) {
			  auto key = dis(gen);
			  wl.addEntry(t, {key, key * 100, t * 1.23});
		  }
	  }
  }

	//wl.shuffle();

	wl.serialize(workload_file);
	workload_file.close();
}
