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

#include <boost/program_options.hpp>

#include "pfabric.hpp"
#include "qcomp/Plan.hpp"
#include "qcomp/QueryCompiler.hpp"
#include "qcomp/SQLParser.hpp"
#include "qcomp/TopologyBuilder.hpp"

using namespace pfabric;
namespace po = boost::program_options;

typedef unsigned int uint_t;

// TransactionID, AccountID, CustomerID, Balance
using AccountPtr = TuplePtr<TransactionID, uint_t, uint_t, double>;
// AccountID, CustomerName, Balance
using ResultPtr = TuplePtr<uint_t, uint_t, double>;

// A state class for chopping the data stream into transactions
struct TxState {
  TxState() : lastTx(0) {}

  TransactionID lastTx;
};

static StateContext<AccountPtr::element_type, uint_t> sCtx{};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " filename" << std::endl;
    return -1;
  }
  PFabricContext ctx;

  /* --- Create the table for storing account information --- */
  TableInfo tblInfo("accounts",
                    {ColumnInfo("LastTxID", ColumnInfo::UInt_Type),
                     ColumnInfo("AccountID", ColumnInfo::UInt_Type),
                     ColumnInfo("CustomerName", ColumnInfo::UInt_Type),
                     ColumnInfo("Balance", ColumnInfo::Double_Type)},
                    ColumnInfo::UInt_Type);

  TableInfo tblInfo2("replica",
                    {ColumnInfo("LastTxID", ColumnInfo::UInt_Type),
                     ColumnInfo("AccountID", ColumnInfo::UInt_Type),
                     ColumnInfo("CustomerName", ColumnInfo::UInt_Type),
                     ColumnInfo("Balance", ColumnInfo::Double_Type)},
                    ColumnInfo::UInt_Type);

  auto accountTable =
      ctx.createMVCCTable<AccountPtr::element_type, uint_t>(tblInfo, sCtx);
  auto replicaTable =
    ctx.createMVCCTable<AccountPtr::element_type, uint_t>(tblInfo2, sCtx);

  std::shared_ptr<MVCCTable<AccountPtr::element_type, uint_t>> tbls[2] = {accountTable, replicaTable};

  /* --- The function for chopping the stream into transactions --- */
  auto txChopping =
      [&](const AccountPtr &tp, bool,
         StatefulMap<AccountPtr, AccountPtr, TxState> &self) -> AccountPtr {
    if (self.state()->lastTx == 0) {
      // we received the first tuple - let's begin a new transaction
      const auto txID = sCtx.newTx(tbls);
      sCtx.tToTX[get<0>(tp)] = txID;
      self.publishPunctuation(
        std::make_shared<Punctuation>(Punctuation::TxBegin, txID, 0));
    } else if (self.state()->lastTx != get<0>(tp)) {
      // we start a new transaction but first commit the previous one
      std::cout << "Commit of tx #" << self.state()->lastTx
                << "(" << sCtx.tToTX[self.state()->lastTx] << ")\n";
      self.publishPunctuation(std::make_shared<Punctuation>(
          Punctuation::TxCommit, sCtx.tToTX[self.state()->lastTx], 0));
      self.state()->lastTx = get<0>(tp);

      // we wait 10 seconds to run another query concurrently
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
      const auto txID = sCtx.newTx(tbls);
      sCtx.tToTX[get<0>(tp)] = txID;
      self.publishPunctuation(
        std::make_shared<Punctuation>(Punctuation::TxBegin, txID, 0));
    }
    self.state()->lastTx = get<0>(tp);
    return tp;
  };

  /* --- Topology #1: Process a transactional data stream --- */
  auto t1 = ctx.createTopology();
  auto s = t1->newStreamFromFile(argv[1])
               .extract<AccountPtr>(',')
               .statefulMap<AccountPtr, TxState>(txChopping)
               .assignTransactionID([](auto tp) { return sCtx.tToTX[get<0>(tp)]; })
               .keyBy<1, uint_t>()
               .toMVCCTable<uint_t>(accountTable)
               .toMVCCTable<uint_t>(replicaTable);
  t1->start();

  /* --- Topology #2: Every 5 seconds print out the accounts table --- */
  auto t2 = ctx.createTopology();
  /*auto d = t2->selectFromMVCCTable<AccountPtr, uint_t>(replicaTable, sCtx.nextTxID)
               .map<ResultPtr>([](auto tp, bool) -> ResultPtr {
                 return makeTuplePtr(get<1>(tp), get<2>(tp), get<3>(tp));
               })
               .print(std::cout);
  */

  uint_t keys[2] = {12, 13};
  auto d = t2->fromMVCCTables<AccountPtr, uint_t>(tbls, keys, sCtx)
    .map<ResultPtr>([](auto tp, bool) -> ResultPtr {
      return makeTuplePtr(get<1>(tp), get<2>(tp), get<3>(tp));
    })
    .print(std::cout);

  t2->runEvery(1);

  t1->wait();

  accountTable->drop();
}
