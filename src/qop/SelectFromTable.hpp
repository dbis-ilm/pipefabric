/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef SelectFromTable_hpp_
#define SelectFromTable_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "table/Table.hpp"

namespace pfabric {

  /**
   * @brief A SelectFromTable operator creates a stream from the tuples
   * of a relational table.
   *
   * The SelectFromTable operator produces a stream of tuples
   * from the given table.
   *
   * @tparam StreamElement
   *    the data stream element type which shall be retrieve from the table
   * @tparam KeyType
   *    the data type of the key for identifying tuples in the table
   */
  template<typename StreamElement, typename KeyType = DefaultKeyType>
  class SelectFromTable : public DataSource<StreamElement> {
  public:
    PFABRIC_SOURCE_TYPEDEFS(StreamElement);

    typedef std::shared_ptr<Table<StreamElement, KeyType>> TablePtr;
    typedef typename Table<StreamElement, KeyType>::Predicate Predicate;


    /**
     * Create a new SelectFromTable operator that produces a stream of tuples
     * from the given table.

     * @param tbl the table that is read
     * @param pred an optional filter predicate
     */
    SelectFromTable(TablePtr tbl, Predicate pred = nullptr) : mTable(tbl) {
    }

    /**
     * Deallocates all resources.
     */
    ~SelectFromTable() {}

    unsigned long start() {
      unsigned long ntuples = 0;

      auto handle = mPredicate == nullptr ? mTable->select() : mTable->select(mPredicate);
      for (auto i = handle.first; i != handle.second; i++) {
        this->getOutputDataChannel().publish(*i, false);
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
