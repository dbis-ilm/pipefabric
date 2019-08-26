#ifndef MVCC_WORKLOADGENERATOR_H
#define MVCC_WORKLOADGENERATOR_H

#include <iostream>
#include <fstream>
#include <list>
#include <vector>

#include "common.h"

namespace pfabric {


template<typename RecordType>
class Workload {
 public:
  struct Entry {
    Entry() {};
    Entry(TransactionID t, const RecordType& v)
      : tx{t}, value{v} {};

    Entry(TransactionID t, RecordType&& v)
      : tx{t}, value{std::move(v)} {};

    TransactionID tx;
    RecordType value;
  };
  using EntryList = std::list<Entry>;

  Workload() : workload(new EntryList) {}

  std::shared_ptr<EntryList> getWorkload() const { return workload; };

  void addEntry(const TransactionID _txnID,
                const RecordType& _val) {
    workload->emplace_back(_txnID, _val);
  }

  void addEntry(const TransactionID _txnID,
                RecordType&& _val) {
    workload->emplace_back(_txnID, std::move(_val));
  }

  void clear() {
    workload.reset(new EntryList);
  }

  void shuffle() {
    std::vector<std::reference_wrapper<const Entry>>
      tmp(workload->begin(), workload->end());
    std::shuffle(tmp.begin(), tmp.end(), gen);
    EntryList shuffled;
    for (auto &ref: tmp)
      shuffled.push_back(std::move(ref.get()));
    workload->swap(shuffled);
  }

  void serialize(std::ostream &stream) const {
    for (const auto &e: *workload)
      stream << e.tx << ',' << e.value << '\n';
  }

  void deserialize(std::istream &stream) {
    TransactionID tx;
    RecordType val;
    while (stream >> tx >> val) {
      workload->emplace_back(tx, val);
    }
  }

 private:
  std::shared_ptr<EntryList> workload;

};

template<bool Z>
void generateWorkload(const double theta, const std::string &name) {
  Workload<ResultPtr::element_type> wl;
  std::ofstream workload_file;

  workload_file.open(name);

  if(Z){
    ZipfianGenerator zipfGen{0, keyRange-1, theta};
    std::cout << "Using Zipf with theta = " << theta << '\n';
    for (auto t = 1u; t < workloadNumTxs+1; ++t) {
      for(auto k = 0u; k < txSize; ++k) {
        auto key = zipfGen.nextValue();
        wl.addEntry(t, {key, dis(gen) * 100, dis(gen) * 1.23});
      }
    }
  } else {

    std::cout << "Using Uni with maximum value = " << uniMax << '\n';
    for (auto t = 1u; t < workloadNumTxs+1; ++t) {
      for(auto k = 0u; k < txSize; ++k) {
        auto key = dis(gen);
        wl.addEntry(t, {key, dis(gen) * 100, dis(gen) * 1.23});
      }
    }
  }

  //wl.shuffle();

  wl.serialize(workload_file);
  workload_file.close();
}
}

#endif //MVCC_WORKLOADGENERATOR_H
