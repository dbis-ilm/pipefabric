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

#ifndef FromMVCCTable_hpp_
#define FromMVCCTable_hpp_

#include <random>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "table/MVCCTable.hpp"

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
  template<typename StreamElement, typename KeyType = DefaultKeyType>
  class FromMVCCTables : public DataSource<StreamElement> {
  public:
    PFABRIC_SOURCE_TYPEDEFS(StreamElement);
    using RecordType = typename StreamElement::element_type;
    using TablePtr = std::shared_ptr<MVCCTable<RecordType, KeyType>>;
    using SCtxType = StateContext<RecordType, KeyType>;
    using Predicate = typename MVCCTable<RecordType, KeyType>::Predicate;


    /**
     * Create a new SelectFromTable operator that produces a stream of tuples
     * from the given table.

     * @param tbl the table that is read
     * @param pred an optional filter predicate
     */
    FromMVCCTables(unsigned int keyRange, SCtxType& sCtx)
      : mTables{sCtx.registeredStates[0], sCtx.registeredStates[1]}, dis{0, keyRange}, mSCtx{sCtx} {}

    /**
     * Deallocates all resources.
     */
    ~FromMVCCTables() {}

    unsigned long start() {
      auto mTxnID = mSCtx.newTx();
//      std::cout << "MyTxnID: " << mTxnID << '\n';
      std::random_device rd;
      std::mt19937 gen(rd());
      const KeyType mKeys[2] = {dis(gen), dis(gen)};
      //std::cout << "Keys: " << mKeys[0] << ", " << mKeys[1] << '\n';

      assert(mTables[0].get() != nullptr);
      assert(mTables[1].get() != nullptr);

      SmartPtr<RecordType> tpls[2][2];

      restart:;
      for (auto i = 0u; i < 2; i++) {
        for (auto j = 0u; j < 2; j++) {
          if (mTables[i]->getByKey(mTxnID, mKeys[j], tpls[i][j]) != 0) {
            /* restart, caused by inconsistency */
            mSCtx.restarts++;
            mTxnID = mSCtx.newTx();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
            //goto restart;
            return 0;
          }
        }
      }
      // when everything consistent, publish the tuples
      //TODO: check if same for correctness criteria
      for (auto i = 0u; i < 2; i++) {
        for (auto j = 0u; j < 2; j++) {
          this->getOutputDataChannel().publish(tpls[i][j], false);
        }
      }

      this->getOutputPunctuationChannel().publish(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
      return 4;
    }

  private:
    const TablePtr mTables[2];      //< the table from which the tuples are fetched
    std::uniform_int_distribution<KeyType> dis;
    SCtxType& mSCtx;

  };

}

#endif
