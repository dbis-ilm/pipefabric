#ifndef MVCC_COMMON_H
#define MVCC_COMMON_H

#include <array>
#include <random>
#include <cassert>

#include "../../pfabric.hpp"

namespace pfabric {

using uint_t = unsigned long long;

/* TransactionID, AccountID, CustomerID, Balance */
using AccountPtr = TuplePtr<TransactionID, uint_t, uint_t, double>;
/* AccountID, CustomerName, Balance */
using ResultPtr = TuplePtr<uint_t, uint_t, double>;

using KeyType = uint_t;

constexpr auto numWriteOps{500 * 1000};
/* Actually twice this constant since each operation accesses two tables */
constexpr auto txSize{5};
constexpr auto workloadNumTxs{numWriteOps / txSize};
constexpr auto simReaders{40};
constexpr auto readInterval{1};
constexpr auto keyRange{1000 * 1000};
constexpr auto repetitions{10};
constexpr auto runs{3};
constexpr auto zipf{true};
//constexpr auto thetas = std::array<double,1>{2.9};
//constexpr auto thetas = std::array<double,3>{1.1,1.3,1.5};
constexpr auto thetas = std::array<double,8>{0.1,0.3,0.5,0.7,0.9,1.1,1.3,1.5};
//constexpr auto thetas = std::array<double,7>{1.7,1.9,2.1,2.3,2.5,2.7,2.9};
//constexpr auto thetas = std::array<double,15>{0.1,0.3,0.5,0.7,0.9,1.1,1.3,1.5,1.7,1.9,2.1,2.3,2.5,2.7,2.9};
constexpr auto uniMax{keyRange-1};
std::mt19937 gen(std::random_device{}());
std::uniform_int_distribution<KeyType> dis(0, uniMax);

constexpr auto resultFile{"results_1mZ10opsNVMFlush.csv"};
constexpr auto resultFileRec{"results_recovery.csv"};

}
#endif /* MVCC_COMMON_H */
