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

// TransactionID, AccountID, CustomerName, Balance
typedef TuplePtr<TransactionID, uint_t, std::string, double> AccountPtr;
// AccountID, CustomerName, Balance
typedef TuplePtr<uint_t, std::string, double> ResultPtr;

// A state class for chopping the data stream into transactions
struct TxState {
  TxState() : lastTx(0) {}

  TransactionID lastTx;
};

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
                     ColumnInfo("CustomerName", ColumnInfo::String_Type),
                     ColumnInfo("Balance", ColumnInfo::Double_Type)},
                    ColumnInfo::UInt_Type);

  auto accountTable =
      ctx.createTxTable<AccountPtr::element_type, uint_t>(tblInfo);

  /* --- The function for chopping the stream into transactions --- */
  auto txChopping =
      [](const AccountPtr &tp, bool,
         StatefulMap<AccountPtr, AccountPtr, TxState> &self) -> AccountPtr {
    auto txID = get<0>(tp);
    if (self.state()->lastTx == 0)
      // we received the first tuple - let's begin a new transaction
      self.publishPunctuation(
          std::make_shared<Punctuation>(Punctuation::TxBegin, txID, 0));
    else if (self.state()->lastTx != txID) {
      // we start a new transaction but first commit the previous one
      std::cout << "Commit of tx #" << self.state()->lastTx << std::endl;
      self.publishPunctuation(std::make_shared<Punctuation>(
          Punctuation::TxCommit, self.state()->lastTx, 0));
      self.state()->lastTx = txID;

      // we wait 10 seconds to run another query concurrently
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10s);
      self.publishPunctuation(
          std::make_shared<Punctuation>(Punctuation::TxBegin, txID, 0));
    }
    self.state()->lastTx = txID;
    return tp;
  };

  /* --- Topology #1: Process a transactional data stream --- */
  auto t1 = ctx.createTopology();
  auto s = t1->newStreamFromFile(argv[1])
               .extract<AccountPtr>(',')
               .statefulMap<AccountPtr, TxState>(txChopping)
               .assignTransactionID([](auto tp) { return get<0>(tp); })
               .keyBy<1, uint_t>()
               .toTxTable<uint_t>(accountTable);
  t1->start();

  /* --- Topology #2: Every 5 seconds print out the accounts table --- */
  auto t2 = ctx.createTopology();
  auto d = t2->selectFromTxTable<AccountPtr, uint_t>(accountTable)
               .map<ResultPtr>([](auto tp, bool) -> ResultPtr {
                 return makeTuplePtr(get<1>(tp), get<2>(tp), get<3>(tp));
               })
               .print(std::cout);
  t2->runEvery(5);

  t1->wait();
}
