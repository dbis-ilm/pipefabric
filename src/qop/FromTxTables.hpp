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
    FromTxTables(SCtxType& sCtx): mTables{sCtx.regStates}, mSCtx{sCtx} {}

    /**
     * Deallocates all resources.
     */
    ~FromTxTables() {}

    unsigned long start() {
      auto mTxnID = mSCtx.newTx();
      mSCtx.txCntR++;
      KeyType mKeys[TxSize];

      if (mSCtx.usingZipf) {
        for (auto i = 0u; i < TxSize; i++)
          mKeys[i] = mSCtx.zipfGen->nextValue();
      } else {
        for (auto i = 0u; i < TxSize; i++)
          mKeys[i] = mSCtx.dis->operator()(mSCtx.rndGen);
      }

      for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
        assert(mTables[s].get() != nullptr);
      }

      SmartPtr<RecordType> tpls[MAX_STATES_TOPO][TxSize];

      auto waitTime = 1u;
      restart:;
      for (auto j = 0u; j < TxSize; ++j) {
        for (auto i = 0u; i < MAX_STATES_TOPO; ++i) {
          if (mTables[i]->getByKey(mTxnID, mKeys[j], tpls[i][j]) != Errc::SUCCESS) {
            /* restart, caused by inconsistency or other erros */
            // std::cout << "Key: " << mKeys[j] << std::endl;
            mSCtx.restarts++;
            for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
              mTables[s]->cleanUpReads(mKeys, (i > s)? j+1 : j);
            }
            mSCtx.setReadCTS(mTxnID, 0, 0);
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(500*TxSize*waitTime));
            // waitTime *= 2;
            //boost::this_thread::interruption_point();
            goto restart;
          }
        }
      }

      /* Only important for BOCC */
      std::array<Errc, MAX_STATES_TOPO> status;
      for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
        status[s] = mTables[s]->readCommit(mTxnID, mKeys, TxSize);
      }
      if (std::any_of(status.begin(), status.end(), [](Errc e){ return e != Errc::SUCCESS;})) {
        mSCtx.restarts++;
        mSCtx.removeTx(mTxnID);
        mTxnID = mSCtx.newTx();
        goto restart;
      }

      /* check if same for correctness criteria *//*
      using KeyType = typename std::tuple_element<1, typename RecordType::Base>::type;
      using ElementType = typename std::tuple_element<2, typename RecordType::Base>::type;
      for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
        std::array<SmartPtr<RecordType>, TxSize> tpls_sep;
        std::copy_n(std::begin(tpls[s]), TxSize, std::begin(tpls_sep));
        std::sort(tpls_sep.begin(), tpls_sep.end(), [](SmartPtr<RecordType> a, SmartPtr<RecordType> b) {
          return std::get<1>(*a) > std::get<1>(*b);
        });
        KeyType prevKey = 0;
        ElementType prevValue = 0;
        for (auto o = 0u; o < TxSize; ++o) {
          if (std::get<1>(*tpls_sep[o]) == prevKey) {
            if (std::get<2>(*tpls_sep[o]) != prevValue) {
              std::cout << "ERROR: INCONSISTENT LOCAL READ\n";
            }
          } else {
            prevKey = std::get<1>(*tpls_sep[o]);
            prevValue = std::get<2>(*tpls_sep[o]);
          }
        }
      }
      for (auto o = 0u; o < TxSize; ++o) {
        std::array<ElementType, MAX_STATES_TOPO> elements;
        for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
          elements[s] = std::get<2>(*tpls[s][o]);
        }
        if (std::any_of(elements.begin()+1, elements.end(), [&](ElementType el) {return el != elements[0];}))
          std::cout << "ERROR: INCONSISTENT ACROSS STATE READ\n";
      }*/

      /* when everything consistent, publish the tuples */
      for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
        for (auto o = 0u; o < TxSize; ++o) {
          this->getOutputDataChannel().publish(tpls[s][o], false);
        }
      }

      this->getOutputPunctuationChannel().publish(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
      for (auto s = 0u; s < MAX_STATES_TOPO; ++s) {
        mTables[s]->cleanUpReads(mKeys, TxSize);
      }
      mSCtx.removeTx(mTxnID);
      return MAX_STATES_TOPO*TxSize;
    }

  private:
    const std::array<TablePtr, MAX_STATES_TOPO> mTables;  //< the table from which the tuples are fetched
    SCtxType& mSCtx;
  };

}

#endif
