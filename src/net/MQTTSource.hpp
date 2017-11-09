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

#ifndef MQTTSource_hpp_
#define MQTTSource_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "mqtt/async_client.h"

namespace pfabric {

  /**
   * @brief MQTTSource is a source operator for receiving tuples via MQTT.
   *
   * A MQTTSource is an operator producing a stream of tuples which are received via MQTT.
   * The operator produces a stream of @c TStringPtr elements.
   */
  class MQTTSource : public DataSource<TStringPtr> {
    public:
      PFABRIC_SOURCE_TYPEDEFS(TStringPtr);

      /**
       * @brief Create a new instance of the MQTTSource operator.
       *
       * Create a new MQTTSource operator for receiving stream tuples via MQTT.
       *
       * @param[in] conn
       *    server connection info, e.g. "tcp://localhost:1883"
       * @param[in] channel
       *    the name of the channel to listen on
       */
      MQTTSource(const std::string& conn, const std::string& channel);

      /**
       * Deallocates all resources.
       */
      ~MQTTSource();

      /**
       * Start the operator by polling the MQTT server.
       *
       * @return always 0
       */
      unsigned long start();

      /**
       * Stop the processing.
       */
      void stop();

    protected:
      mqtt::async_client *cli;
      std::string chan;

      void produceTuple(const StringRef& data);
      void producePunctuation(PunctuationPtr pp);
  };
}

#endif
