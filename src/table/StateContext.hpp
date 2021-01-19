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
#include <libpmem.h>
#endif

namespace pfabric {

using TableID = unsigned short;

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
class ZipfianGenerator {
  public:
    static constexpr double ZIPFIAN_CONSTANT = 0.99;
    ZipfianGenerator(unsigned int min, unsigned int max, double zipfianconstant);
    unsigned int nextValue();

  private:
    unsigned int nextInt(unsigned int itemcount);

    /** Number of items. */
    const unsigned int items;

    /** Min item to generate. */
    const unsigned int base;

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
  using TopoGrp   = std::pair<std::array<TablePtr,2>, std::atomic<LastCTS>>;
  using WriteInfo = std::array<Status,2>; //std::tuple<TableID, Status>;
  using ReadInfo  = std::array<ReadCTS,1>; //std::tuple<TopologyID, ReadCTS>;
  using ActiveTx  = std::tuple<TransactionID, WriteInfo, ReadInfo>;

 public:
  /** Atomic counter for assigning global transaction IDs */
  std::atomic<TransactionID> nextTxID{1};
  /** Registered States and Topology Groups (currently hard-coded, not thread-safe) */
  TablePtr regStates[2];
  TopoGrp topoGrps[1];
  /** Mapping from internal transaction ID to gloabl transaction ID */
  std::unordered_map<TransactionID, TransactionID> tToTX;

  /*---- Only for evaluation -------------------------------------------------*/
  /** Counting necessary restarts of txs */
  std::atomic_ullong restarts{0};
  std::atomic_ullong txCntR{0};
  std::atomic_ullong txCntW{0};
  /** Generating random keys */
  std::mt19937 rndGen{std::random_device{}()};
  /** Distribution settings */
  bool usingZipf = false;
  std::unique_ptr<ZipfianGenerator> zipfGen;
  std::unique_ptr<std::uniform_int_distribution<KeyType>> dis;

  void setDistribution(bool zipf, KeyType min, KeyType max, double zipfConst = 0.0) {
    usingZipf = zipf;
    dis.reset(new std::uniform_int_distribution<KeyType>{min, max});
    if(zipf) {
      zipfGen.reset(new ZipfianGenerator{min, max, zipfConst});
    }
  }
  /*--------------------------------------------------------------------------*/

  /** Get status of a writing transaction; either active, commit or abort */
  const Status &getWriteStatus(const TransactionID txnID,
                               const TableID tblID) const {
    return std::get<1>(activeTxs[getPosFromTxnID(txnID)])[tblID];
  }

  Status &getWriteStatus(const TransactionID txnID, const TableID tblID) {
    return std::get<1>(activeTxs[getPosFromTxnID(txnID)])[tblID];
  }

  /** Get status of a reading transaction; returns read snapshot version */
  const ReadCTS getReadCTS(const TransactionID txnID,
                           const GroupID topoID) const {
    return std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID];
  }

  ReadCTS& getReadCTS(const TransactionID txnID, const GroupID topoID) {
    return std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID];
  }

  /** Set status of a reading transaction */
  void setReadCTS(const TransactionID txnID, const GroupID topoID, const ReadCTS read) {
    std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID] = read;
  }

  const TransactionID getOldestActiveTx() const {
      auto oldest = DTS_INF;
      const auto slots = usedSlots.load(std::memory_order_relaxed);
      for(int pos = 0; pos < 64; ++pos) {
        const auto bTS = std::get<0>(activeTxs[pos]);
        if((slots & (1ULL << pos)) && bTS < oldest)
          oldest = bTS;
      }
      return oldest;
  }

  /** Registers a new transaction to the context */
  const TransactionID newTx() {
    const auto txnID = nextTxID.fetch_add(1);
    const auto pos = getSetFreePos(usedSlots);
    activeTxs[pos] = std::make_tuple(txnID,
        WriteInfo{{Status::Active, Status::Active}}, //< TableID | Status
        ReadInfo{{0}});                              //< GroupID | LastCommitID
    return txnID;
  }

  const TransactionID getNewTS() {
    return nextTxID.fetch_add(1);
  }

  /** Removes a transaction from the context; possibly has to recalculate the
   *  oldest visible version*/
  void removeTx(const TransactionID txnID) {
    const auto readCTS = getReadCTS(txnID, 0);
    setReadCTS(txnID, 0, 0);
    unsetPos(usedSlots, getPosFromTxnID(txnID)); //< release slot
    TransactionID min = oldestVisibleVersion.load(std::memory_order_relaxed);

    /* find new minimum */
    if(min != 0) {
      auto newMin = DTS_INF;
      const auto slots = usedSlots.load(std::memory_order_relaxed);
      for(int pos = 0; pos < 64; ++pos) {
        const auto rCTS = std::get<2>(activeTxs[pos])[0];
        if((slots & (1ULL << pos)) && rCTS != 0 && rCTS < newMin)
          newMin = rCTS;
      }
      /* no other active Tx, use last Snapshot */
      if (newMin == DTS_INF) newMin = getLastCTS(0);
      while(min < newMin && !oldestVisibleVersion.compare_exchange_weak(min, newMin, std::memory_order_relaxed));
    } else if(min == 0) {
      const auto newMin = getLastCTS(0);
      while(!oldestVisibleVersion.compare_exchange_weak(min, newMin, std::memory_order_relaxed));
    }
  }

  /** Get last committed transaction ID (snapshot version) */
  const TransactionID getLastCTS(const GroupID topoID) {
    return topoGrps[topoID].second.load(std::memory_order_relaxed);
  }

  /** Set last committed transaction ID (snapshot version) */
  void setLastCTS(const GroupID topoID, const TransactionID txnID) {
#ifdef USE_NVM_TABLES
    pmem_drain();
    topoGrps[topoID].second.store(txnID, std::memory_order_relaxed);
    pmem_persist(&topoGrps[topoID].second,  sizeof(TransactionID));
#else
    topoGrps[topoID].second.store(txnID, std::memory_order_relaxed);
#endif
  }

  /** Get oldest currently visible version; used for garbage collection */
  const TransactionID getOldestVisible() const{
    return oldestVisibleVersion.load(std::memory_order_relaxed);
  }

  /** Register a new state/table to the context */
  const TableID registerState(const TablePtr tbl) {
    regStates[numStates] = tbl;
    return numStates++;
  }

  /** Register a new topology/continuous query to the context */
  const GroupID registerTopo(const std::array<TablePtr,2> &tbls) {
    topoGrps[numGroups] = std::make_pair(tbls, 0);
    return numGroups++;
  }

  void reset() {
    /* Make sure no thread is using the context anymore! */
    nextTxID.store(1);
    restarts.store(0);
    txCntR.store(0);
    txCntW.store(0);
    usedSlots.store(0);
    oldestVisibleVersion.store(1);
    topoGrps[0].second.store(0);
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
  GroupID numGroups{0u};
};

} /* end namespace pfabric */

#endif /* end ifndef StateContext_hpp_ */
