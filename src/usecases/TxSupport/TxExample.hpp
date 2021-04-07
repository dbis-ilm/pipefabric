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

/**
 * This demo illustrates transactional data stream processing. One topology
 * produces a stream of elements which consists of individual transactions
 * marked by BEGIN and COMMIT. The stream elements are used to update a
 * relational table. A second batch topology (query) reads this table
 * periodically. The transactional context guarantees snapshot isolation
 * of this query.
 */
#ifndef TX_EXAMPLE_H
#define TX_EXAMPLE_H

#include <chrono>
#include <cmath>
#include <limits>
#include <iomanip>

#include "common.h"
#include "Workload.h"

using namespace pfabric;

template<typename TableType>
class TxExample {
  public:
    TxExample(const std::string& _pName, const bool _tpsScaling) noexcept
      : sCtx{}, pName{_pName}, tpsScaling{_tpsScaling} {}

    void run() {
      double theta = 0.0;
      std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
      start = std::chrono::high_resolution_clock::now();
      end = std::chrono::high_resolution_clock::now();
      auto overhead = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

      ofstream resFile, resFileRec;
      resFile.open(resultFile, ios::out | ios::app);
      resFileRec.open(resultFileRec, ios::out | ios::app);

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

      start = std::chrono::high_resolution_clock::now();
      auto accountTable = ctx.createTxTable<TableType>(tblInfo, sCtx);
      end = std::chrono::high_resolution_clock::now();
      resFileRec << pName << ',' << keyRange << ',' << simReaders << ",State 1 Recovery,"
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() - overhead
        << '\n';
      start = std::chrono::high_resolution_clock::now();
      auto replicaTable = ctx.createTxTable<TableType>(tblInfo2, sCtx);
      end = std::chrono::high_resolution_clock::now();
      resFileRec << pName << ',' << keyRange << ',' << simReaders << ",State 2 Recovery,"
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() - overhead
        << '\n';

      std::vector<typename std::chrono::duration<int64_t, std::milli>::rep> measures;
      std::vector<TransactionID> txnCnt;
      std::vector<TransactionID> txnCntR;
      std::vector<TransactionID> txnCntW;
      std::vector<uint64_t> restarts;

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
                std::make_shared<Punctuation>(Punctuation::TxBegin, txID, Timestamp(0)));
          } else if (self.state()->lastTx != get<0>(tp)) {
            /* we start a new transaction but first commit the previous one */
            self.publishPunctuation(std::make_shared<Punctuation>(
                  Punctuation::TxCommit, sCtx.tToTX[self.state()->lastTx], Timestamp(0)));
            self.state()->lastTx = get<0>(tp);

            const auto txID = sCtx.newTx();
            sCtx.tToTX[get<0>(tp)] = txID;
            self.publishPunctuation(
                std::make_shared<Punctuation>(Punctuation::TxBegin, txID, Timestamp(0)));
          }
          self.state()->lastTx = get<0>(tp);
          return tp;
        };


      /*==========================================================================*
       * Topology #1: Writer transactional data stream                            *
       *==========================================================================*/
      /// If LastCTS of our topology group > 0 then we recovered from an existing state context
      start = std::chrono::high_resolution_clock::now();
      if ((*sCtx.topoGrps)[0].second > 0)
        sCtx.updateTopo(0, {accountTable->getID(), replicaTable->getID()});
      else
        sCtx.registerTopo({accountTable->getID(), replicaTable->getID()});
      end = std::chrono::high_resolution_clock::now();
      resFileRec << pName << ',' << keyRange << ',' << simReaders << ",Update Pointers,"
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() - overhead
        << '\n';
      /*auto tWriter = ctx.createTopology();
      auto s = tWriter->newStreamFromMemory<AccountPtr>(zipf? "wl_writes_zipf.csv" : "wl_writes_uni.csv")
        .statefulMap<AccountPtr, TxState>(txChopping)
        .assignTransactionID([&](auto tp) { return sCtx.tToTX[get<0>(tp)]; })
        .template keyBy<1, uint_t>()
        .template toTxTable<TableType>(accountTable)
        .template toTxTable<TableType>(replicaTable)
        // .print(std::cout)
        ;*/

      /*==========================================================================*
       * Topology #2: Readers concurrently/consistent access to both tables       *
       *==========================================================================*/
      start = std::chrono::high_resolution_clock::now();
      PFabricContext::TopologyPtr tReaders[simReaders];
      for(auto i = 0u; i < simReaders; i++) {
        tReaders[i] = ctx.createTopology();
        auto d = tReaders[i]->fromTxTables<TableType, AccountPtr, txSize>(sCtx)
          .template map<ResultPtr>([](auto tp, bool) -> ResultPtr {
              return makeTuplePtr(get < 1 > (tp), get < 2 > (tp), get < 3 > (tp));
              })
        //.print(std::cout)
        ;
      }
      end = std::chrono::high_resolution_clock::now();
      resFileRec << pName << ',' << keyRange << ',' << simReaders << ",Recreate Reader Queries,"
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() - overhead
        << '\n';



      /*==========================================================================*
       * Prepare Tables                                                           *
       *==========================================================================*/
      auto prepareTables = [&]() {
        accountTable->truncate();
        replicaTable->truncate();
        // auto start = std::chrono::high_resolution_clock::now();
        const auto txID = sCtx.newTx();
        accountTable->transactionBegin(txID);
        replicaTable->transactionBegin(txID);
        for (auto i = 0u; i < keyRange; i++) {
          accountTable->insert(txID, i, {txID, i, i * 100, i * 1.0});
          replicaTable->insert(txID, i, {txID, i, i * 100, i * 1.0});
        }
        // auto end = std::chrono::high_resolution_clock::now();
        // auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // std::cout << "Insert time: " << diff << "ms\n";

        // start = std::chrono::high_resolution_clock::now();
        accountTable->transactionPreCommit(txID);
        replicaTable->transactionPreCommit(txID);
        // end = std::chrono::high_resolution_clock::now();
        // diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        //  std::cout << "Commit time: " << diff << "ms\n";

        // std::cout << "Created Tables with " << accountTable->size() << " elements each.\n\n";
      };

      /*==========================================================================*
       * Run Topologies                                                           *
       *==========================================================================*/
      auto runTopologies = [&]() {
        for (auto j = 0u; j < runs; j++) {
          std::cout << "\rRun " << j+1 << '/' << runs << std::flush;
          prepareTables();

          /* Necessary to clear streamFromMemory data vector */
          start = std::chrono::high_resolution_clock::now();
          auto tWriter = ctx.createTopology();
          tWriter->newStreamFromMemory<AccountPtr>(zipf? "wl_writes_zipf.csv" : "wl_writes_uni.csv")
            .statefulMap<AccountPtr, TxState>(txChopping)
            .assignTransactionID([&](auto tp) { return sCtx.tToTX[get<0>(tp)]; })
            .template keyBy<1, uint_t>()
            .template toTxTable<TableType>(accountTable)
            .template toTxTable<TableType>(replicaTable);
            //.print(std::cout);
          end = std::chrono::high_resolution_clock::now();
          resFileRec << pName << ',' << keyRange << ',' << simReaders << ",Recreate Writer Query,"
            << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() - overhead
            << '\n';

          tWriter->prepare();
          const auto txnIDBefore = sCtx.nextTxID.load();

          start = std::chrono::high_resolution_clock::now();

          tWriter->start(true);
          for (const auto &t : tReaders) t->runEvery(readInterval);
          tWriter->wait();

          end = std::chrono::high_resolution_clock::now();
          auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

          measures.push_back(diff);
          txnCnt.push_back(sCtx.nextTxID.load() - txnIDBefore);
          restarts.push_back(sCtx.restarts.load());
          txnCntR.push_back(sCtx.txCntR.load());
          txnCntW.push_back(sCtx.txCntW.load());

          tWriter->cleanStartupFunctions();
          for (const auto &t : tReaders) t->stopThreads();
          sCtx.reset();
        }
        std::cout << '\n';
      };

      /*==========================================================================*
       * Accumulate Measures                                                      *
       *==========================================================================*/
      auto accumulateMeasures = [&]() {
        //const auto avg = std::accumulate(measures.begin(), measures.end(), 0) / measures.size();
        /* Scaling for BOCC as each transaction requests 5 timestamps: 1 Start, 2 Val, 2 End.
         * Two because for each table a validation and end time stamp is requested */
        const auto throughput = tpsScaling?
          (((std::accumulate(txnCnt.begin(), txnCnt.end(), 0) - std::accumulate(restarts.begin(), restarts.end(), 0)) / 5) *
           1000ULL / std::accumulate(measures.begin(), measures.end(), 0))
          :
          (std::accumulate(txnCnt.begin(), txnCnt.end(), 0) *
           1000ULL / std::accumulate(measures.begin(), measures.end(), 0));
        const auto errors = std::accumulate(restarts.begin(), restarts.end(), 0) * 100.0 /
          std::accumulate(txnCnt.begin(), txnCnt.end(), 0);

        const auto rTp =
          (std::accumulate(txnCntR.begin(), txnCntR.end(), 0) *
           1000ULL / std::accumulate(measures.begin(), measures.end(), 0));
        const auto wTp=
          (std::accumulate(txnCntW.begin(), txnCntW.end(), 0) / 2 *
           1000ULL / std::accumulate(measures.begin(), measures.end(), 0));

        /*
        std::cout << "Results:"
         //  << "\n\tTime: " << avg << "ms"
           << "\n\tThroughput: " << throughput << "tx/s"
           << "\n\tError Rate: " << errors << "%\n";

           std::cout << "Read Tps: " << rTp << "tx/s, "
           << "Write Tps: " << wTp << "tx/s\n";
        */

        /* protocol,table_size,transaction_size,readers,contention,throughput,error_rate */
        resFile << pName << ',' << keyRange << ',' << txSize*2 << ',' << simReaders << ',' << theta << ',' << "read" << ',' << rTp << ',' << errors << '\n';
        resFile << pName << ',' << keyRange << ',' << txSize*2 << ',' << simReaders << ',' << theta << ',' << "write" << ',' << wTp << ',' << errors << '\n';

        measures.clear();
        txnCnt.clear();
        txnCntR.clear();
        txnCntW.clear();
        restarts.clear();
      };


      /*==========================================================================*
       * Execution                                                                *
       *==========================================================================*/
      if constexpr (zipf) {
        for(auto t = 0u; t < thetas.size(); t++) {
          theta = thetas[t];
          sCtx.setDistribution(zipf, 0, keyRange-1, theta);
          for (auto i = 0u; i < repetitions; ++i) {
            generateWorkload<zipf>(theta, "wl_writes_zipf.csv");
            //prepareTables();
            runTopologies();
            accumulateMeasures();
            resFile.flush();
          }
        }
      } else {
        generateWorkload<zipf>(0, "wl_writes_uni.csv");
        sCtx.setDistribution(zipf, 0, keyRange-1);
        for (auto i = 0u; i < repetitions; ++i) {
          runTopologies();
          accumulateMeasures();
        }
      }
      resFile.close();

    } /* run */

  private:

    /* A state class for chopping the data stream into transactions */
    struct TxState {
      TxState() : lastTx(0) {}
      TransactionID lastTx;
    };

    StateContext<TableType> sCtx;
    const std::string pName;
    const bool tpsScaling;
};

#endif
