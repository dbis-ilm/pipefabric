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

#ifndef SelectFromTxTable_hpp_
#define SelectFromTxTable_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "table/TxTable.hpp"

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
  class SelectFromTxTable : public DataSource<StreamElement> {
  public:
    PFABRIC_SOURCE_TYPEDEFS(StreamElement);

    typedef std::shared_ptr<TxTable<typename StreamElement::element_type, KeyType>> TablePtr;
    typedef typename TxTable<typename StreamElement::element_type, KeyType>::Predicate Predicate;


    /**
     * Create a new SelectFromTable operator that produces a stream of tuples
     * from the given table.

     * @param tbl the table that is read
     * @param pred an optional filter predicate
     */
    SelectFromTxTable(TablePtr tbl, Predicate pred = nullptr) : mTable(tbl) {
    }

    /**
     * Deallocates all resources.
     */
    ~SelectFromTxTable() {}

    unsigned long start() {
      unsigned long ntuples = 0;

      assert(mTable.get() != nullptr);

      auto iter = mPredicate == nullptr ? mTable->select() : mTable->select(mPredicate);
      for (; iter.isValid(); iter++) {
        auto tup = *iter;
        this->getOutputDataChannel().publish(tup, false);
        ntuples++;
      }

      // publish punctuation
      this->getOutputPunctuationChannel().publish(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
      return ntuples;
    }

  private:
    TablePtr mTable;      //< the table from which the tuples are fetched
    Predicate mPredicate; //< a predicate for filtering tuples
  };

}

#endif
