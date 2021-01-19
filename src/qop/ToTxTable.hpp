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

#ifndef ToTxTable_hpp_
#define ToTxTable_hpp_

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"
#include "table/Table.hpp"

namespace pfabric {

  /**
   * @brief A ToTable operator stores stream elements in a given relational table.
   *
   * The ToTable operator is a stream operator which stores tuples arriving
   * from a stream in a given relational table. Depending on the existence
   * of the key value, the tuple is newly inserted or an existing tuple is
   * updated.
   *
   * @tparam TableType
   *    the type of the underlying transactional table
   * @tparam StreamElement
   *    the data stream element type which shall be stored in the table
   */
  template<
    typename TableType,
    typename StreamElement
  >
  class ToTxTable : public UnaryTransform<StreamElement, StreamElement> {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);
    using KeyType = typename TableType::KType;

  public:
    /** Alias for a pointer to the table. */
    using TablePtr = std::shared_ptr<TableType>;

    /** the function for deriving the key of an incoming stream element */
    using KeyFunc = std::function<KeyType(const StreamElement&)>;

    /** the function for deriving the TransactionID of an incoming stream element */
    using TxIDFunc = std::function<TransactionID(const StreamElement&)>;
 
    /**
     * Create a new ToTable operator to store incoming tuples in the
     * given table.
     *
     * @param tbl pointer to the table object
     * @param func function pointer for deriving the key of the tuple
     * @param autoCommit auto-commit mode
     */
    ToTxTable(TablePtr tbl, KeyFunc keyFunc, TxIDFunc txFunc, bool autoCommit = false) :
      mTable(tbl), mKeyFunc(keyFunc), mTxFunc(txFunc), mAutoCommit(autoCommit) {}

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, ToTxTable, processDataElement);

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, ToTxTable, processPunctuation);

    const std::string opName() const override { return std::string("ToTxTable"); }

  private:

    /**
     * @brief This method is invoked when a punctuation arrives.
     *
     * It simply forwards the @c punctuation to the subscribers.
     *
     * @param[in] punctuation
     *    the incoming punctuation tuple
     */
    void processPunctuation(const PunctuationPtr& punctuation) {
      if (punctuation->ptype() == Punctuation::TxCommit)
        mTable->transactionPreCommit(boost::any_cast<TransactionID>(punctuation->data()));
      else if (punctuation->ptype() == Punctuation::TxAbort)
        mTable->transactionAbort(boost::any_cast<TransactionID>(punctuation->data()));
      else if (punctuation->ptype() == Punctuation::TxBegin)
        mTable->transactionBegin(boost::any_cast<TransactionID>(punctuation->data()));

      this->getOutputPunctuationChannel().publish(punctuation);
    }

    /**
     * @brief This method is invoked when a stream element arrives from the publisher.
     *
     * It inserts or updates the tuple in the table. If the tuple is outdated
     * it will be removed instead.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement(const StreamElement& data, const bool outdated) {
      auto txID = mTxFunc(data);

      auto key = mKeyFunc(data);
      if (outdated)
        mTable->deleteByKey(txID, key);
      else {
        mTable->insert(txID, key, *data);
      }
      if (mAutoCommit) {
        // perform commit
        mTable->transactionCommit(txID);
      }
      this->getOutputDataChannel().publish(data, outdated);
    }

    TablePtr mTable;  //< function pointer to the table where tuples will be stored
    KeyFunc mKeyFunc; //< pointer to the key extractor function
    TxIDFunc mTxFunc; //< pointer to the TransactionID extractor func
    bool mAutoCommit; //< auto-commit mode
  };

} // namespace pfabric

#endif
