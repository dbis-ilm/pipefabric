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

/**
 * This demo illustrates transactional data stream processing. One topology
 * produces a stream of elements which consists of individual transactions
 * marked by BEGIN and COMMIT. The stream elements are used to update a
 * relational table. A second batch topology (query) reads this table
 * periodically. The transactional context guarantees snapshot isolation
 * of this query.
 */
#include <chrono>
#include <cmath>
#include <limits>
#include <iomanip>

#include "pfabric.hpp"
#include "common.h"

using namespace pfabric;

using TableType = S2PLTable<AccountPtr::element_type, uint_t>;

// A state class for chopping the data stream into transactions
struct TxState {
  TxState() : lastTx(0) {}
  TransactionID lastTx;
};

static StateContext<TableType> sCtx{};

int main() {
  sCtx.keyRange = keyRange;
  sCtx.uniMax = uniMax;
  if(zipf) sCtx.zipfConst = zipfTheta;

  PFabricContext ctx;
  /* --- Create the table for storing account information --- */
  TableInfo tblInfo("accounts", {
                     ColumnInfo("LastTxID", ColumnInfo::UInt_Type),
                     ColumnInfo("AccountID", ColumnInfo::UInt_Type),
                     ColumnInfo("CustomerName", ColumnInfo::UInt_Type),
                     ColumnInfo("Balance", ColumnInfo::Double_Type)},
                    ColumnInfo::UInt_Type);

  TableInfo tblInfo2("replica", {
                      ColumnInfo("LastTxID", ColumnInfo::UInt_Type),
                      ColumnInfo("AccountID", ColumnInfo::UInt_Type),
                      ColumnInfo("CustomerName", ColumnInfo::UInt_Type),
                      ColumnInfo("Balance", ColumnInfo::Double_Type)},
                     ColumnInfo::UInt_Type);

  auto accountTable =
    ctx.createS2PLTable<AccountPtr::element_type, uint_t>(tblInfo, sCtx);
  auto replicaTable =
    ctx.createS2PLTable<AccountPtr::element_type, uint_t>(tblInfo2, sCtx);

  /*==========================================================================*
   * The function for chopping the stream into transactions                   *
   *==========================================================================*/
  auto txChopping =
    [&](const AccountPtr &tp, bool,
        StatefulMap<AccountPtr, AccountPtr, TxState> &self) -> AccountPtr {
      if (self.state()->lastTx == 0 || sCtx.tToTX.empty()) {
        /* we received the first tuple - let's begin a new transaction */
        self.state()->lastTx = 0;
        const auto txID = sCtx.newTx();
        sCtx.tToTX[get<0>(tp)] = txID;
        self.publishPunctuation(
          std::make_shared<Punctuation>(Punctuation::TxBegin, txID, 0));
      } else if (self.state()->lastTx != get<0>(tp)) {
        /* we start a new transaction but first commit the previous one */
        self.publishPunctuation(std::make_shared<Punctuation>(
          Punctuation::TxCommit, sCtx.tToTX[self.state()->lastTx], 0));
        self.state()->lastTx = get<0>(tp);

        const auto txID = sCtx.newTx();
        sCtx.tToTX[get<0>(tp)] = txID;
        self.publishPunctuation(
          std::make_shared<Punctuation>(Punctuation::TxBegin, txID, 0));
      }
      self.state()->lastTx = get<0>(tp);
      return tp;
    };

  /*==========================================================================*
   * Topology #1: Writer transactional data stream                            *
   *==========================================================================*/
  sCtx.registerTopo({accountTable, replicaTable});
  auto tWriter = ctx.createTopology();
  auto s = tWriter->newStreamFromMemory<AccountPtr>(zipf? "wl_writes_zipf.csv" : "wl_writes_uni.csv")
    .statefulMap<AccountPtr, TxState>(txChopping)
    .assignTransactionID([&](auto tp) { return sCtx.tToTX[get<0>(tp)]; })
    .keyBy<1, uint_t>()
    .toTxTable<TableType>(accountTable)
    .toTxTable<TableType>(replicaTable)
    //.print(std::cout)
    ;

  /*==========================================================================*
   * Topology #2: Readers concurrently/consistent access to both tables       *
   *==========================================================================*/
  PFabricContext::TopologyPtr tReaders[simReaders];
  for(auto i = 0u; i < simReaders; i++) {
    tReaders[i] = ctx.createTopology();
    auto d = tReaders[i]->fromTxTables<TableType, AccountPtr, txSize>(sCtx)
      .map<ResultPtr>([](auto tp, bool) -> ResultPtr {
        return makeTuplePtr(get < 1 > (tp), get < 2 > (tp), get < 3 > (tp));
      })
    //.print(std::cout)
    ;
  }

  /*==========================================================================*
   * Prepare Tables                                                           *
   *==========================================================================*/
  auto prepareTables = [&]() {
    auto start = std::chrono::high_resolution_clock::now();
    const auto txID = sCtx.newTx();
    for (auto i = 0u; i < keyRange; i++) {
      accountTable->insert(txID, i, {txID, i, i * 100, i * 1.0});
      replicaTable->insert(txID, i, {txID, i, i * 100, i * 1.0});
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();
    std::cout << "Insert time: " << diff << "ms\n";

    start = std::chrono::high_resolution_clock::now();
    accountTable->transactionPreCommit(txID);
    replicaTable->transactionPreCommit(txID);
    end = std::chrono::high_resolution_clock::now();
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();
    std::cout << "Commit time: " << diff << "ms\n";
   
    std::cout << "Created Tables with " << accountTable->size() << " elements each.\n\n";
  };

  std::vector<typename std::chrono::duration<int64_t, std::milli>::rep> measures;
  std::vector<TransactionID> txnCnt;
  std::vector<unsigned int> restarts;

  /*==========================================================================*
   * Run Topologies                                                           *
   *==========================================================================*/
  for (auto j = 0u; j < repetitions; j++) {
    std::cout << "Run " << j+1 << '/' << repetitions << '\n';
    prepareTables();
    tWriter->prepare();

    auto start = std::chrono::high_resolution_clock::now();

    tWriter->start(true);
    for (const auto &t : tReaders) t->runEvery(readInterval);
    tWriter->wait();

    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    measures.push_back(diff);
    txnCnt.push_back(sCtx.nextTxID.load(memory_order_relaxed));
    restarts.push_back(sCtx.restarts.load(memory_order_relaxed));

    tWriter->cleanStartupFunctions();
    for (const auto &t : tReaders) t->stopThreads();
    sCtx.reset();
    accountTable->drop();
    replicaTable->drop();
  }


  /*==========================================================================*
   * Accumulate Measures                                                      *
   *==========================================================================*/
  const auto avg = std::accumulate(measures.begin(), measures.end(), 0) / measures.size();
  const auto throughput = std::accumulate(txnCnt.begin(), txnCnt.end(), 0) * 1000ULL / std::accumulate(measures.begin(), measures.end(), 0);
  const auto errors = std::accumulate(restarts.begin(), restarts.end(), 0) * 100.0 / std::accumulate(txnCnt.begin(), txnCnt.end(), 0);

  std::cout << "Results:"
            << "\n\tTime: " << avg << "ms"
            << "\n\tThroughput: " << throughput << "tx/s"
            << "\n\tError Rate: " << errors << "%\n";
}
