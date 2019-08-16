/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef SelectFromMVCCTable_hpp_
#define SelectFromMVCCTable_hpp_

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
  class SelectFromMVCCTable : public DataSource<StreamElement> {
  public:
    PFABRIC_SOURCE_TYPEDEFS(StreamElement);
    using RecordType = typename StreamElement::element_type;
    using TablePtr = std::shared_ptr<MVCCTable<RecordType, KeyType>>;
    using Predicate = typename MVCCTable<RecordType, KeyType>::Predicate;


    /**
     * Create a new SelectFromTable operator that produces a stream of tuples
     * from the given table.

     * @param tbl the table that is read
     * @param pred an optional filter predicate
     */
    SelectFromMVCCTable(TablePtr tbl, std::atomic<TransactionID>& aCnter, Predicate pred = nullptr)
      : mTable(tbl), mACnter(aCnter), mPredicate(pred) {}

    /**
     * Deallocates all resources.
     */
    ~SelectFromMVCCTable() {}

    unsigned long start() {
      auto mTxnID = mACnter.fetch_add(1);
      std::cout << "MyTxnID: " << mTxnID << '\n';

      unsigned long ntuples = 0;

      assert(mTable.get() != nullptr);

      auto iter = mPredicate == nullptr ? mTable->select() : mTable->select(mPredicate);
      for (; iter.isValid(); iter++) {
        const auto& mvccObj = get<0>(*iter);
        auto tup = SmartPtr<RecordType>(new RecordType(mvccObj.values[mvccObj.getCurrent(mTxnID)]));
        this->getOutputDataChannel().publish(tup, false);
        ntuples++;
      }

      // publish punctuation
      this->getOutputPunctuationChannel().publish(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
      return ntuples;
    }

  private:
    TablePtr mTable;      //< the table from which the tuples are fetched
    std::atomic<TransactionID>& mACnter;
    Predicate mPredicate; //< a predicate for filtering tuples
  };

}

#endif
