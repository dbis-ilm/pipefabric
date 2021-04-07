/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef StateContext_hpp_
#define StateContext_hpp_

#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <random>
#include <unordered_map>

#include "core/PFabricTypes.hpp"
#ifdef USE_NVM_TABLES
#include "pfabric_config.h"
#include <libpmem.h>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#endif

namespace pfabric {

#ifdef USE_NVM_TABLES
using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::transaction;
#endif
using TableID = unsigned short;

/// Settings, TODO: maybe these should rather be template arguments
constexpr auto MAX_TOPO_GRPS =   1; ///< number of allowed topology groups
constexpr auto MAX_STATES =      2; ///< number of globally allowed states
constexpr auto MAX_STATES_TOPO = 2; ///< number of allowed states per topology group

/** Infinity, used for maximum validity */
constexpr auto DTS_INF = std::numeric_limits<TransactionID>::max();
/** Possible isolation levels */
enum class IsoLevel {READ_COMMITTED, SNAPSHOT, SERIALIZABLE};
/** Custom error codes */
enum class Errc {SUCCESS, ABORT, NOT_FOUND, INCONSISTENT};
/** Possible Transaction states */
enum class Status {Active, Commit, Abort};

/** Forward declarations */
uint8_t getFreePos(const uint64_t v);
uint8_t getSetFreePos(std::atomic<std::uint64_t> &v);
void unsetPos(std::atomic<std::uint64_t> &v, const uint8_t pos);
unsigned int hashMe(unsigned int x);

/** Derived from YCSB.
 *  see: https://github.com/brianfrankcooper/YCSB/blob/master/core/src/main/java/com/yahoo/ycsb/generator/ZipfianGenerator.java
 */
template<typename T>
class ZipfianGenerator {
  public:
    static constexpr double ZIPFIAN_CONSTANT = 0.99;
    ZipfianGenerator(T min, T max, double zipfianconstant = ZIPFIAN_CONSTANT)
        : items{max - min + 1}, base{min}, zipfianconstant{zipfianconstant},
          theta{zipfianconstant} {
      for(auto i = 0Lu; i < items; i++)
        zetan += 1 / (std::pow(i + 1, theta));
      for(auto i = 0Lu; i < 2; i++)
        zeta2theta += 1 / (std::pow(i + 1, theta));
      alpha = 1.0 / (1.0 - theta);
      eta = (1 - std::pow(2.0 / items, 1 - theta)) / (1 - zeta2theta / zetan);
      nextValue();
    }

    /* Scrambled version */
    T nextValue() {
      auto ret = nextInt(items);
      return base + 1 + hashMe(ret) % (items-1); ///TODO: Key 0 bugs and is excluded for now
    }

  private:
    T nextInt(size_t itemcount) {
      double u = dist(gen);
      double uz = u * zetan;

      if (uz < 1.0) { return base;}
      if (uz < 1.0 + std::pow(0.5, theta)) { return base + 1; }
      return base + (int) ((itemcount) * std::pow(eta * u - eta + 1, alpha));
    }

    /** Number of items. */
    const size_t items;

    /** Min item to generate. */
    const T base;

    /** The zipfian constant to use. */
    const double zipfianconstant;

    /** Computed parameters for generating the distribution. */
    double alpha, zetan{0}, eta, theta, zeta2theta{0};

    std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<> dist{0.0 ,1.0};
};

/*******************************************************************************
 * @brief State Context to track the status of the states and provide
 *        transactional guarantees
 *        TODO: get rid of hardcoded stuff (array sizes and so on)
 ******************************************************************************/
template <typename TableType>
class StateContext {
  using KeyType   = typename TableType::KType;
  using ReadCTS   = TransactionID;
  using LastCTS   = TransactionID;
  using GroupID   = unsigned short;
  using TablePtr  = std::shared_ptr<TableType>;
  using TopoGrp   = std::pair<std::array<TableID, MAX_STATES_TOPO>, std::atomic<LastCTS>>;
  using WriteInfo = std::array<Status, MAX_STATES_TOPO>; //std::tuple<TableID, Status>;
  using ReadInfo  = std::array<ReadCTS, MAX_TOPO_GRPS>; //std::tuple<TopologyID, ReadCTS>;
  using ActiveTx  = std::tuple<TransactionID, WriteInfo, ReadInfo>;

 public:
  /** Atomic counter for assigning global transaction IDs */
  std::atomic<TransactionID> nextTxID{
    std::chrono::duration_cast<std::chrono::duration<long long unsigned int, std::nano>>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count()
  };
  /** Registered States and Topology Groups (currently hard-coded, not thread-safe) */
#ifdef USE_NVM_TABLES
  struct sCtxRoot {
    persistent_ptr<std::array<TopoGrp, MAX_TOPO_GRPS>> topoGrps;
    GroupID numGrps;
  };
  pool<sCtxRoot> pop;
#endif
  std::array<TopoGrp, MAX_TOPO_GRPS> * topoGrps;
  std::array<TablePtr, MAX_STATES> regStates;
  /** Mapping from internal transaction ID to gloabl transaction ID */
  std::unordered_map<TransactionID, TransactionID> tToTX;
  std::mutex mtx{};

  /*---- Only for evaluation -------------------------------------------------*/
  /** Counting necessary restarts of txs */
  std::atomic_ullong restarts{0};
  std::atomic_ullong txCntR{0};
  std::atomic_ullong txCntW{0};
  /** Generating random keys */
  std::mt19937 rndGen{std::random_device{}()};
  /** Distribution settings */
  bool usingZipf = false;
  std::unique_ptr<ZipfianGenerator<KeyType>> zipfGen;
  std::unique_ptr<std::uniform_int_distribution<KeyType>> dis;

  void setDistribution(bool zipf, KeyType min, KeyType max, double zipfConst = 0.0) {
    usingZipf = zipf;
    dis.reset(new std::uniform_int_distribution<KeyType>{min, max});
    if(zipf) {
      zipfGen.reset(new ZipfianGenerator<KeyType>{min, max, zipfConst});
    }
  }
  /*--------------------------------------------------------------------------*/

  StateContext() {
#ifdef USE_NVM_TABLES
    const std::string path = pfabric::gPmemPath + "StateContext"; ///TODO: needs a unique ID
    if (access(path.c_str(), F_OK) != 0) {
      pop = pool<sCtxRoot>::create(path, "StateContext");
      transaction::run(pop, [&] {
        pop.root()->topoGrps = make_persistent<std::array<TopoGrp, MAX_TOPO_GRPS>>();
        pop.root()->numGrps = 0;
      });
    } else {
      pop = pool<sCtxRoot>::open(path, "StateContext");
    }
    topoGrps = pop.root()->topoGrps.get();
    numGroups = &pop.root()->numGrps;
#else
    topoGrps = new std::array<TopoGrp, MAX_TOPO_GRPS>;
    numGroups = new GroupID{0u};
#endif
  }

  ~StateContext() {
#ifdef USE_NVM_TABLES
    pop.close();
#else
    delete topoGrps;
#endif
  }

  /** Get status of a writing transaction; either active, commit or abort */
  const Status &getWriteStatus(const TransactionID txnID, const TableID tblID) const {
    return std::get<1>(activeTxs[getPosFromTxnID(txnID)])[tblID];
  }

  Status &getWriteStatus(const TransactionID txnID, const TableID tblID) {
    return std::get<1>(activeTxs[getPosFromTxnID(txnID)])[tblID];
  }

  /** Get status of a reading transaction; returns read snapshot version */
  ReadCTS getReadCTS(const TransactionID txnID, const GroupID topoID) const {
    return std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID];
  }

  ReadCTS& getReadCTS(const TransactionID txnID, const GroupID topoID) {
    return std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID];
  }

  /** Set status of a reading transaction */
  void setReadCTS(const TransactionID txnID, const GroupID topoID, const ReadCTS read) {
    std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID] = read;
  }

  TransactionID getOldestActiveTx() const {
      auto oldest = DTS_INF;
      const auto &slots = usedSlots.load(std::memory_order_relaxed);
      for(auto pos = 0u; pos < 64; ++pos) {
        if (slots & (1ULL << pos)) {
          const auto bTS = std::get<0>(activeTxs[pos]);
          oldest = std::min(bTS, oldest);
        }
      }
      return oldest;
  }

  /** Registers a new transaction to the context */
  TransactionID newTx() {
    const auto txnID = nextTxID.fetch_add(1);
    const auto pos = getSetFreePos(usedSlots);
    activeTxs[pos] = std::make_tuple(txnID, WriteInfo{}, ReadInfo{});
    std::get<1>(activeTxs[pos]).fill(Status::Active); ///< TableID | Status
    std::get<2>(activeTxs[pos]).fill(0);              ///< GroupID | LastCommitID
    return txnID;
  }

  TransactionID getNewTS() {
    return nextTxID.fetch_add(1);
  }

  /** Removes a transaction from the context */
  void removeTx(const TransactionID txnID) {
    for (auto topo = 0u; topo < MAX_TOPO_GRPS; ++topo)
      setReadCTS(txnID, topo, 0); ///< reset ReadCTSs for next transaction
    unsetPos(usedSlots, getPosFromTxnID(txnID)); //< release slot
  }

  /** Recalculate the oldest visible version */
  TransactionID recalcOldestVisible(const TransactionID txnID) {
    TransactionID min = oldestVisibleVersion.load(std::memory_order_relaxed);
    auto newMin = DTS_INF;

    /* find new minimum */
    if(min != 0) {
      const auto slots = usedSlots.load(std::memory_order_relaxed);
      for(auto pos = 0u; pos < 64; ++pos) {
        if (slots & (1ULL << pos)) {
          const auto rCTS = std::get<2>(activeTxs[pos])[0];
          if (rCTS != 0 && rCTS < newMin)
            newMin = rCTS;
        }
      }
      /* no other active Tx, use last Snapshot */
      if (newMin == DTS_INF) newMin = getLastCTS(0);
      while(min < newMin && !oldestVisibleVersion.compare_exchange_weak(min, newMin, std::memory_order_relaxed));
    } else if(min == 0) {
      newMin = getLastCTS(0);
      while(!oldestVisibleVersion.compare_exchange_weak(min, newMin, std::memory_order_relaxed));
    }
    return newMin;
  }

  /** Get last committed transaction ID (snapshot version) */
  TransactionID getLastCTS(const GroupID topoID) {
    return (*topoGrps)[topoID].second.load(std::memory_order_relaxed);
  }

  /** Set last committed transaction ID (snapshot version) */
  void setLastCTS(const GroupID topoID, const TransactionID txnID) {
#ifdef USE_NVM_TABLES
    pmem_drain();
    (*topoGrps)[topoID].second.store(txnID, std::memory_order_relaxed);
    pmem_persist(&(*topoGrps)[topoID].second,  sizeof(TransactionID));
#else
    (*topoGrps)[topoID].second.store(txnID, std::memory_order_relaxed);
#endif
  }

  /** Get oldest currently visible version; used for garbage collection */
  TransactionID getOldestVisible() const {
    return oldestVisibleVersion.load(std::memory_order_relaxed);
  }

  /** Register a new state/table to the context */
  TableID registerState(const TablePtr tbl) {
    regStates[numStates] = tbl;
    return numStates++;
  }

  /** Register a new topology/continuous query to the context */
  GroupID registerTopo(const std::array<TableID, MAX_STATES_TOPO> &tbls) {
    auto &numGrps = *numGroups;
    (*topoGrps)[numGrps] = std::make_pair(tbls, 0);
#ifdef USE_NVM_TABLES
    pmem_flush(&(*topoGrps)[numGrps], sizeof(TableID) * MAX_STATES_TOPO + sizeof(LastCTS));
    ++numGrps;
    pmem_persist(numGroups, sizeof(GroupID));
#else
    ++numGrps;
#endif
    return numGrps-1;
  }

  /** Update the table ID of an existing topology group */
  void updateTopo(const GroupID topoID, const std::array<TableID, MAX_STATES_TOPO> &tbls) {
    (*topoGrps)[topoID].first = tbls; ///< the tableIDs doesn't need to be persistent
  }


  void reset() {
    /* Make sure no thread is using the context anymore! */
    nextTxID.store(
      std::chrono::duration_cast<std::chrono::duration<long long unsigned int, std::nano>>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    restarts.store(0);
    txCntR.store(0);
    txCntW.store(0);
    usedSlots.store(0);
    oldestVisibleVersion.store(0);
    tToTX.clear();
  }

 private:

  /** calculate and return the position in the activeTxs array for the given
   *  transaction ID; */
  const uint8_t getPosFromTxnID(const TransactionID txnID) const {
    auto pos = -1;
    while(std::get<0>(activeTxs[++pos]) != txnID);
    return pos;
  }

  std::atomic<std::uint64_t> usedSlots;
  std::array<ActiveTx, 64> activeTxs;
  /** oldest considered version by active reading transactions,
   *  used for cleaning up version arrays */
  std::atomic<TransactionID> oldestVisibleVersion{0};
  TableID numStates{0u};
  GroupID * numGroups;
};

} /* end namespace pfabric */

#endif /* end ifndef StateContext_hpp_ */
