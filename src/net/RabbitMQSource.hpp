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

#ifndef RabbitMQSource_hpp_
#define RabbitMQSource_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "AMQPcpp.h"

namespace pfabric {

  /**
   * @brief RabbitMQSource is a source operator for receiving tuples via AMQP interface.
   *
   * A RabbitMQSource is an operator producing a stream of tuples which are received
   * via AMQP interface. The operator produces a stream of @c TStringPtr elements,
   * where each element corresponds to a single AMQP message.
   */
  class RabbitMQSource : public DataSource<TStringPtr> {
    public:
      PFABRIC_SOURCE_TYPEDEFS(TStringPtr);

      /**
       * @brief Create a new instance of the RabbitMQSource operator.
       *
       * Create a new RabbitMQSource operator for receiving stream tuples via AMQP.
       *
       * @param[in] info
       *    a string containing password, user, address and port of the server
       *    format: "password:user@address:port", e.g. "guest:guest@localhost:5672"
       * @param[in] queueName
       *    a string containing the name of the queue for exchanging tuples, e.g. "queue"
       */
      RabbitMQSource(const std::string& info, const std::string& queueName);

      /**
       * Deallocates all resources.
       */
      ~RabbitMQSource();

      /**
       * Start the operator by listing at the given port and address.
       *
       * @return always 0
       */
      unsigned long start();

      /**
       * Stop the processing.
       */
      void stop();

    protected:
      std::string mInfo;
      std::string mQueueName;

      void produceTuple(const StringRef& data);
      void producePunctuation(PunctuationPtr pp);
  };
}

#endif
