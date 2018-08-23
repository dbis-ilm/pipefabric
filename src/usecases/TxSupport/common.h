#ifndef MVCC_COMMON_H
#define MVCC_COMMON_H

#include <random>
#include <cassert>
#include "pfabric.hpp"

namespace pfabric {
using uint_t = unsigned int;

/* TransactionID, AccountID, CustomerID, Balance */
using AccountPtr = TuplePtr<TransactionID, uint_t, uint_t, double>;
/* AccountID, CustomerName, Balance */
using ResultPtr = TuplePtr<uint_t, uint_t, double>;

using KeyType = uint_t;//std::tuple_element_t<1, typename AccountPtr::element_type>;

constexpr auto numWriteOps = 500 * 1000;
/* Actually twice this constant since each operation accesses two tables */
constexpr auto txSize = 5;
constexpr auto workloadNumTxs = numWriteOps / txSize;
constexpr auto simReaders = 23;
constexpr auto readInterval = 1;
constexpr auto keyRange = 100 * 1000;
constexpr auto repetitions = 3;
constexpr auto zipf = true;
constexpr auto zipfTheta = 0.9;
constexpr auto uniMax = 10;

std::mt19937 gen(std::random_device{}());
std::uniform_int_distribution<KeyType> dis(0, uniMax);

}
#endif /* MVCC_COMMON_H */
