/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
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

/** Helper */
uint8_t getFreePos(const uint64_t v);
uint8_t getSetFreePos(std::atomic<std::uint64_t> &v);
void unsetPos(std::atomic<std::uint64_t> &v, const uint8_t pos);

/*******************************************************************************
 * @brief State Context to track the status of the states and provide
 *        transactional guarantees
 *        TODO: get rid of hardcoded stuff (array sizes and so on)
 ******************************************************************************/
template <typename TableType>
class StateContext {
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
  std::atomic_uint restarts{0};
  /** Generating random keys */
  std::mt19937 rndGen{std::random_device{}()};
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

  /** Set status of a reading transaction */
  void setReadCTS(const TransactionID txnID, const GroupID topoID, const ReadCTS read) {
    std::get<2>(activeTxs[getPosFromTxnID(txnID)])[topoID] = read;
  }
  
  /** Registers a new transaction to the context */
  const TransactionID newTx() {
    const auto txnID = nextTxID.fetch_add(1);
    const auto pos = getSetFreePos(usedSlots);
    activeTxs[pos] = std::make_tuple(txnID,
        WriteInfo{Status::Active, Status::Active}, //< TableID | Status
        ReadInfo{0});                              //< GroupID | LastCommitID
    return txnID;
  }

  /** Removes a transaction from the context; possibly has to recalculate the
   *  oldest visible version*/
  void removeTx(const TransactionID txnID) {
    unsetPos(usedSlots, getPosFromTxnID(txnID)); //< release slot
    
    const auto readCTS = getReadCTS(txnID, 0);
    TransactionID min = oldestVisibleVersion.load();

    /* find new minimum */
    if (readCTS != 0 && min >= readCTS) {
      auto newMin = DTS_INF;
      const auto slots = usedSlots.load();
      for(int pos = 0; pos < 64; pos++) {
        const auto rCTS = std::get<2>(activeTxs[pos])[0];
        if((slots & (1ULL << pos)) && rCTS != 0 && rCTS < newMin)
          newMin = rCTS;
      }
      /* no other active Tx, use last Snapshot */
      if (newMin == DTS_INF) newMin = getLastCTS(0); 
      while(min < newMin && !oldestVisibleVersion.compare_exchange_weak(min, newMin));
    }
  }

  /** Get last committed transaction ID (snapshot version) */
  const TransactionID getLastCTS(const GroupID topoID) {
    return topoGrps[topoID].second.load();
  }

  /** Set last committed transaction ID (snapshot version) */
  void setLastCTS(const GroupID topoID, const TransactionID txnID) {
    topoGrps[topoID].second.store(txnID);
  }

  /** Get oldest currently visible version; used for garbage collection */
  const TransactionID getOldestVisible() const{
    return oldestVisibleVersion.load();
  }

  /** Register a new state/table to the context */
  const TableID registerState(const TablePtr tbl) {
    regStates[numStates] = tbl;
    return numStates++;
  }

  /** Register a new topology/continuous query to the context */
  const GroupID registerTopo(const std::array<TablePtr,2> &tbls) {
    topoGrps[numGroups] = std::make_pair(tbls, 1);
    return numGroups++;
  }

  void reset() {
    /* Make sure no thread is using the context anymore! */
    nextTxID.store(1);
    restarts.store(0);
    usedSlots.store(0);
    tToTX.clear();
    regStates[0] = nullptr;
    regStates[1] = nullptr;
    numStates = 0u;
    topoGrps[0] = nullptr;
    numGroups = 0u;
  }

 private:

  /** calculate and return the position in the activeTxs array for the given
   *  transaction ID; TODO: can this be more efficient? */
  const uint8_t getPosFromTxnID(const TransactionID txnID) const {
    const auto slots = usedSlots.load();
    for(int pos = 0; pos < 64; pos++) {
      if(slots & (1ULL << pos) && std::get<0>(activeTxs[pos]) == txnID)
        return pos;
    }
    return 64;
  }

  std::atomic<std::uint64_t> usedSlots;
  std::array<ActiveTx, 64> activeTxs;
  /** oldest considered version by active reading transactions,
   *  used for cleaning up version arrays */
  std::atomic<TransactionID> oldestVisibleVersion{1};
  TableID numStates{0u};
  GroupID numGroups{0u};
};

} /* end namespace pfabric */

#endif /* end ifndef StateContext_hpp_ */
