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

#ifndef FromTxTable_hpp_
#define FromTxTable_hpp_

#include <random>
#include <sstream>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "table/Table.hpp"
#include "table/StateContext.hpp"

namespace pfabric {

 /**
   * @brief A SelectFromTable operator creates a stream from the tuples
   * of a relational table.
   *
   * The SelectFromTable operator produces a stream of tuples
   * from the given table which can be optinally selected by a predicate.
   *
   * @tparam StreamElement
   *    the data stream element type which shall be retrieve from the table
   * @tparam KeyType
   *    the data type of the key for identifying tuples in the table
   */
  template<typename TableType, typename StreamElement, size_t TxSize>
  class FromTxTables : public DataSource<StreamElement> {
  public:
    PFABRIC_SOURCE_TYPEDEFS(StreamElement);
    using RecordType = typename TableType::RType;
    using KeyType = typename TableType::KType;
    using TablePtr = std::shared_ptr<TableType>;
    using SCtxType = StateContext<TableType>;
    using Predicate = typename TableType::Predicate;

    /**
     * Create a new SelectFromTable operator that produces a stream of tuples
     * from the given table.

     * @param tbl the table that is read
     * @param pred an optional filter predicate
     */
    FromTxTables(SCtxType& sCtx): mTables{sCtx.regStates[0], sCtx.regStates[1]}, mSCtx{sCtx} {}

    /**
     * Deallocates all resources.
     */
    ~FromTxTables() {}

    unsigned long start() {
      auto mTxnID = mSCtx.newTx();
      mSCtx.txCntR++;
      KeyType mKeys[TxSize];

      if (mSCtx.usingZipf) {
        for(auto i = 0u; i < TxSize; i++)
          mKeys[i] = mSCtx.zipfGen->nextValue();
      } else {
        for(auto i = 0u; i < TxSize; i++)
          mKeys[i] = mSCtx.dis->operator()(mSCtx.rndGen);
      }

      assert(mTables[0].get() != nullptr);
      assert(mTables[1].get() != nullptr);

      SmartPtr<RecordType> tpls[2][TxSize];
      
      auto waitTime = 1u;
      restart:;
      for (auto j = 0u; j < TxSize; j++) {
        for (auto i = 0u; i < 2; i++) {
          if (mTables[i]->getByKey(mTxnID, mKeys[j], tpls[i][j]) != Errc::SUCCESS) {
            /* restart, caused by inconsistency or other erros */
//            std::cout << "Key: " << mKeys[j] << std::endl;
            mSCtx.restarts++;
            mTables[0]->cleanUpReads(mKeys, i?j+1:j);
            mTables[1]->cleanUpReads(mKeys, j);
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(500*TxSize*waitTime));
//            waitTime *= 2;
//            boost::this_thread::interruption_point();
            goto restart;
          }
        }
      }
      
      /* Only important for BOCC */
      const auto s1 = mTables[0]->readCommit(mTxnID, mKeys, TxSize);
      const auto s2 = mTables[1]->readCommit(mTxnID, mKeys, TxSize);
      if(s1 != Errc::SUCCESS || s2 != Errc::SUCCESS) {
        mSCtx.restarts++;
        mSCtx.removeTx(mTxnID);
        mTxnID = mSCtx.newTx();
        goto restart;
      }

      /* check if same for correctness criteria */
//      for (auto j = 0u; j < TxSize; j++) {
//        if (std::get<2>(*tpls[0][j]) != std::get<2>(*tpls[1][j]))
//          std::cout << "ERROR: INCONSISTENT READ\n";
//      }

      /* when everything consistent, publish the tuples */
      for (auto i = 0u; i < 2; i++) {
        for (auto j = 0u; j < TxSize; j++) {
          this->getOutputDataChannel().publish(tpls[i][j], false);
        }
      }

      this->getOutputPunctuationChannel().publish(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
      mTables[0]->cleanUpReads(mKeys, TxSize);
      mTables[1]->cleanUpReads(mKeys, TxSize);
      mSCtx.removeTx(mTxnID);
      return 2*TxSize;
    }

  private:
    const TablePtr mTables[2];      //< the table from which the tuples are fetched
    SCtxType& mSCtx;
  };

}

#endif
