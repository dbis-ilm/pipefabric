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

#ifndef ZMQSink_hpp_
#define ZMQSink_hpp_

#include "net/ZMQSocket.hpp"

#include "core/Tuple.hpp"
#include "pubsub/Sink.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/DataSink.hpp"

namespace pfabric {

  /**
   * @brief A sink operator (publisher) using 0MQ to send a tuple stream to other network nodes.
   *
   * A ZMQSink is sink operator (publisher) that uses a 0MQ socket and the publish-subscribe
   * pattern to send a stream of tuples to other nodes.
   *
   * @tparam StreamElement
	 *    the data stream element type consumed by the sink
   */
  template<typename StreamElement>
  class ZMQSink : public SynchronizedDataSink<StreamElement> {
  private:
    PFABRIC_SYNC_SINK_TYPEDEFS(StreamElement)

  public:
    /**
     * Constructs a new 0MQ sink operator to send tuples to other nodes.
     *
     * @param path the path (endpoint) describing the socket (see 0MQ documentation for details)
     * @param type the type of communication pattern (publish-subscribe, push-pull)
     * @param mode the encoding mode for messages (binary, ascii, ...)
     * @param tlen the (optional) length of the tuple (in bytes) used for allocating a buffer
     */
    ZMQSink(const std::string& path, ZMQParams::SinkType stype = ZMQParams::PublisherSink,
       ZMQParams::EncodingMode mode = ZMQParams::BinaryMode, unsigned int tlen = 1024) :
        mMode(mode), mSinkType(stype) {
      if (mSinkType == ZMQParams::PublisherSink) {
        mSocket = std::make_unique< sock::ZMQSocket >(path, ZMQ_PUB);
        // mSync = new ZMQSinkSync(path);
      }
      else if (mSinkType == ZMQParams::PushSink)
        mSocket = std::make_unique< sock::ZMQSocket >(path, ZMQ_PUSH);
      if (mMode == ZMQParams::BinaryMode)
        mBuf.resize(tlen);
    }

    /**
     * Destroy the operator.
     */

    ~ZMQSink() {
      mSocket->closeSocket();
    }

    /**
     * @brief Bind the callback for the data channel.
     */
     BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, ZMQSink, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, ZMQSink, processPunctuation );

    private:

      /**
       * Send a punctuation tuple via 0MQ depending on the encoding.
       *
       * @param data the punctuation tuple
       */
      void processPunctuation(const PunctuationPtr& data) {
        if (mMode == ZMQParams::BinaryMode) {
          mBuf.clear();
          data->serializeToStream(mBuf);
          mSocket->sendBuffer(mBuf);
        }
        else {
          std::stringstream str;
          // TODO
          mSocket->sendString(str.str());
        }
      }

      /**
       * Send a stream element via 0MQ depending on the encoding.
       *
       * @param data the stream element
       * @param outdated true if the stream element is outdated
       */
      void processDataElement(const StreamElement& data, const bool outdated) {
        /*
        if (mSinkType == ZMQParams::PublisherSink && !mSync->hasSubscriber()) {
          mSync->sync();
          mSync->start();
        }
        */
        if (mMode == ZMQParams::BinaryMode) {
          mBuf.clear();
          // TODO: how to handle outdated tuples?
          data->serializeToStream(mBuf);
          mSocket->sendBuffer(mBuf);
          }
        else {
          std::stringstream str;
          // TODO
          mSocket->sendString(str.str());
        }
      }

    typedef std::unique_ptr< sock::ZMQSocket > ZMQSocketPtr;

    ZMQSocketPtr mSocket;            //< the receiver socket
    StreamType mBuf;                 //< buffer for encoding tuple data
    ZMQParams::EncodingMode mMode;   //< the mode for encoding messages
    // ZMQSinkSync* mSync;              //<
    ZMQParams::SinkType mSinkType;   //< the type of sink (pull or subscriber)
  };

}

#endif
