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
   * @tparam StreamElement
   *    the data stream element type which shall be stored in the table
   * @tparam KeyType
   *    the data type of the key for identifying tuples in the table
   */
  template<
    typename StreamElement,
    typename KeyType = DefaultKeyType
  >
  class ToTxTable : public UnaryTransform<StreamElement, StreamElement> {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

  public:
    //< Typedef for a pointer to the table.
    typedef std::shared_ptr<TxTable<typename StreamElement::element_type, KeyType>> TablePtr;

    //< the function for deriving the key of an incoming stream element
  	typedef std::function< KeyType(const StreamElement&) > KeyFunc;

    //< the function for deriving the TransactionID of an incoming stream element
  	typedef std::function< TransactionID(const StreamElement&) > TxIDFunc;
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
        mTable->transactionCommit(boost::any_cast<TransactionID>(punctuation->data()));
      else if (punctuation->ptype() == Punctuation::TxAbort)
        mTable->transactionAbort(boost::any_cast<TransactionID>(punctuation->data()));

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
