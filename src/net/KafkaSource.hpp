/*
 * Copyright (c) 2014-17 The PipeFabric team,
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

#ifndef KafkaSource_hpp_
#define KafkaSource_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "cppkafka/consumer.h"
#include "cppkafka/configuration.h"

namespace pfabric {

  /**
   * @brief KafkaSource is a source operator for receiving tuples via Apache Kafka protocol.
   *
   * A KafkaSource is an operator producing a stream of tuples which are received via Apache
   * Kafka protocol. The operator produces a stream of @c TStringPtr elements.
   */
  class KafkaSource : public DataSource<TStringPtr> {
    public:
      PFABRIC_SOURCE_TYPEDEFS(TStringPtr);

      /**
       * @brief Create a new instance of the KafkaSource operator.
       *
       * Create a new KafkaSource operator for receiving stream tuples via Apache Kafka
       * protocol.
       *
       * @param[in] broker
       *    the node(s) where the Kafka server runs on
       * @param[in] topic
       *    the topic where the data is stored
       * @param[in] groupID
       *    the ID of the group the consumer belongs to
       */
      KafkaSource(const std::string& broker, const std::string& topic, const std::string& groupID);

      /**
       * Deallocates all resources.
       */
      ~KafkaSource();

      /**
       * Start the operator by polling the Kafka server.
       *
       * @return always 0
       */
      unsigned long start();

      /**
       * Stop the processing.
       */
      void stop();

    protected:
      cppkafka::Consumer *consumer;
      cppkafka::Message msg;

      void produceTuple(const StringRef& data);
      void producePunctuation(PunctuationPtr pp);
  };
}

#endif
