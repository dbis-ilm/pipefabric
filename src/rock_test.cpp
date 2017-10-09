//
// Created by phil on 11.09.17.
//

#include <vector>
#include "pfabric.hpp"

using namespace pfabric;

typedef pfabric::Tuple<int, int, string, double> MyTuple;
template<typename RecordType, typename KeyType>
using LTable = RDBTable<RecordType, KeyType>;

int main() {
  std::chrono::high_resolution_clock::time_point start, end;
  std::vector<typename std::chrono::duration<int64_t, micro>::rep> measures;


  auto testTable = std::make_shared<LTable<MyTuple, int>>("MyTestTable1");
  for (int i = 0; i < 10000; i++) {
    auto tp = MyTuple(i + 1,
                      (i + 1) * 100,
                      fmt::format("String#{}", i),
                      i * 12.345);
    start = std::chrono::high_resolution_clock::now();
    testTable->insert(i, tp);
    end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    measures.push_back(diff);
  }


  auto avg = std::accumulate(measures.begin(), measures.end(), 0) / measures.size();
  auto minmax = std::minmax_element(std::begin(measures), std::end(measures));
  std::cout << "\nInsert Statistics in µs: "
            << "\n\tAverage: " << avg
            << "\n\tMin: " << *minmax.first
            << "\n\tMax: " << *minmax.second << '\n';

  auto ptp = testTable->getByKey(5);
  std::cout << "Tuple 5: " << ptp << '\n';


  /* Clean up */
  testTable->drop();
}

