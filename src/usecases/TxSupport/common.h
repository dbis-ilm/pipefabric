#ifndef MVCC_COMMON_H
#define MVCC_COMMON_H

#include <random>
#include <cassert>
#include "pfabric.hpp"

namespace pfabric {
typedef unsigned int uint_t;

// TransactionID, AccountID, CustomerID, Balance
using AccountPtr = TuplePtr<TransactionID, uint_t, uint_t, double>;
// AccountID, CustomerName, Balance
using ResultPtr = TuplePtr<uint_t, uint_t, double>;

using KeyType = uint_t;//std::tuple_element_t<1, typename AccountPtr::element_type>;

constexpr auto workloadNumTxs = 100 * 1000;
constexpr auto txSize = 2;
constexpr auto simReaders = 3;
constexpr auto readInterval = 10;
constexpr auto keyRange = workloadNumTxs;
constexpr auto repetitions = 1;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<KeyType> dis(0, keyRange - 1);

}
#endif /* MVCC_COMMON_H */