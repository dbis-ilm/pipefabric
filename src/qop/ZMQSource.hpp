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

#ifndef ZMQSource_hpp_
#define ZMQSource_hpp_

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "qop/TextFileSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"

#include "net/ZMQSocket.hpp"

namespace pfabric {

/**
 * Typedefs for a tuple containing only a byte array for serializing tuples.
 */
typedef Tuple<StreamType> TBuf;
typedef TuplePtr<StreamType> TBufPtr;

/**
 * ZMQSourceImpl provides the basic implementation of ZMQSource to
 * receive tuples from socket.
 */
class ZMQSourceImpl {
public:
  typedef std::function< void(TStringPtr) > TStringCallbackFunc;
  typedef std::function< void(TBufPtr) > BufCallbackFunc;
  typedef std::function< void(PunctuationPtr) > PunctuationCallbackFunc;

	/**
	 * Constructor for a ZMQSource implementation to assign a path for the socket,
   * the type of source, and the encoding.
   *
	 * @param path  the socket path where we receive the tuples
	 * @param stype the type of 0MQ source (pull or sink)
	 * @param emode the type of encoding (binary or ascii)
   * @param cb1 a callback function for receiving string tuples
   * @param cb2 a callback function for receiving serialized tuples
   * @param cb3 a callback function for receiving punctuations
	 */
  ZMQSourceImpl(const std::string& path,
    ZMQParams::SourceType stype,
    ZMQParams::EncodingMode emode,
		TStringCallbackFunc const& cb1,
    BufCallbackFunc const& cb2,
    PunctuationCallbackFunc const& cb3);

	/**
	 * Destructor for releasing resources.
	 */
	~ZMQSourceImpl();

	/**
	 * Start the processing.
	 */
	void start();

	/**
	 * Stop the processing.
	 */
	void stop();

	/**
	 * Check whether the processing was interrupted or not.
   *
   * @return true if processing was interrupted
	 */
	bool isInterrupted() const;

private:

	/**
	 * Try to receive and process incoming tuples.
   *
	 * @return number of received tuples
	 */
	unsigned long process();

  typedef std::unique_ptr< sock::ZMQSocket > ZMQSocketPtr;

 	TStringCallbackFunc mTStringCB;          //<
	BufCallbackFunc mBufCB;                  //<
	PunctuationCallbackFunc mPunctuationCB;  //<

	ZMQSocketPtr mSocket;                //< the subscriber socket
	int mNumTuples;                      //< number tuple processed by the socket
	ZMQParams::EncodingMode mMode;       //< the encoding mode
	bool mInterrupted;                   //< a flag for interrupting
  std::thread mSourceThread;           //< the socket  reader thread
	mutable std::mutex mZmqMtx;          //<
	mutable std::mutex mStartMtx;        //<
	ZMQParams::SourceType mType;         //<
};

 /**
  * ZMQSourceBase is the parametric base class for all ZMQSource classes
  * providing only the basic functionality of an internal ZMQSourceBase
  * instance.
  *
  * @tparam Tout
  *         the data stream element type which is produced by the source
  */
	template<typename Tout>
	class ZMQSourceBase : public DataSource<Tout> {
    PFABRIC_SOURCE_TYPEDEFS(Tout)

	public:
		/**
		 * Constructor to create a ZMQSourceBase object delegating the
     * actual processing to a ZMQSourceImpl instance.
     *
     * @param path  the socket path where we receive the tuples
  	 * @param stype the type of 0MQ source (pull or sink)
  	 * @param emode the type of encoding (binary or ascii)
     * @param cb1 a callback function for receiving string tuples
     * @param cb2 a callback function for receiving serialized tuples
	 */
		ZMQSourceBase(const std::string& path,
              ZMQParams::SourceType stype = ZMQParams::SubscriberSource,
							 ZMQParams::EncodingMode emode = ZMQParams::BinaryMode,
               ZMQSourceImpl::TStringCallbackFunc const& cb1 = nullptr,
               ZMQSourceImpl::BufCallbackFunc const& cb2 = nullptr) :
			mImpl(new ZMQSourceImpl( path, stype, emode,
        cb1, cb2,
        std::bind(&ZMQSourceBase::publishPunctuation, this, std::placeholders::_1))) {}

    /**
     * Stop the processing.
     */
		void stop() {
			mImpl->stop();
		}

    /**
     * Start the processing.
     *
     * @return always 0
     */
    unsigned long start() {
      mImpl->start();
      return 0;
    }

	protected:

    /**
     * Produce and forward a punctuation tuple. This method is used as callback
     * for @c ZMQSourceImpl.
     *
     * @param pp the punctuation tuple
     */
		void publishPunctuation(PunctuationPtr pp) {
			this->getOutputPunctuationChannel().publish(pp);
		}

	private:
		std::unique_ptr< ZMQSourceImpl > mImpl; //< pointer to the actual implementation
	};

  /**
   * ZMQSource is a source operator for receiving tuples via 0MQ and produce
   * a stream of tuples. ZMQSource is a template class that can be type specialized
   * for different wire formats ("encoding").
   *
   * @tparam T
   *         the data stream element type which is produced by the source
   */
  template<typename T>
  class ZMQSource : public ZMQSourceBase<T> {};

  /**
   * A type specialized implementation of ZMQSource for serialized tuples, i.e.
   * tuples which are serialized into a byte array.
   */
  template<>
  class ZMQSource<TBufPtr>  : public ZMQSourceBase<TBufPtr> {
    PFABRIC_SOURCE_TYPEDEFS(TBufPtr)

  public:
    /**
     * Constructor to create a ZMQSource for serialized tuples.
     *
     * @param path  the socket path where we receive the tuples
     * @param stype the type of 0MQ source (pull or sink)
     */
    ZMQSource(const std::string& path, ZMQParams::SourceType stype = ZMQParams::SubscriberSource) :
      ZMQSourceBase<TBufPtr>(path, stype, ZMQParams::BinaryMode,
        nullptr,
        std::bind(&ZMQSource<TBufPtr>::publishTuple, this, std::placeholders::_1)) {}

    /**
     * Forward the tuple to all subscribers. This method is used as callback
     * for @c ZMQSourceImpl.
     *
     * @param tp a serialized tuple that is published
     */
    void publishTuple(TBufPtr tp) {
    	this->getOutputDataChannel().publish(tp, false);
    }


  };

  /**
   * A type specialized implementation of ZMQSource for string tuples, i.e.
   * tuples which are consists only of a single text line.
   */
  template<>
  class ZMQSource<TStringPtr> : public ZMQSourceBase<TStringPtr> {
    PFABRIC_SOURCE_TYPEDEFS(TStringPtr)

  public:
    /**
     * Constructor to create a ZMQSource object for string tuples.
     *
     * @param path  the socket path where we receive the tuples
     * @param stype the type of 0MQ source (pull or sink)
   */
    ZMQSource(const std::string& path, ZMQParams::SourceType stype = ZMQParams::SubscriberSource) :
    ZMQSourceBase<TStringPtr>(path, stype, ZMQParams::AsciiMode,
      std::bind(&ZMQSource<TStringPtr>::publishTuple, this, std::placeholders::_1),
    nullptr) {}

    /**
     * Forward the tuple to all subscribers. This method is used as callback
     * for @c ZMQSourceImpl.
     *
     * @param tp a string tuple that is published
     */
    void publishTuple(TStringPtr tp) {
      this->getOutputDataChannel().publish(tp, false);
    }

  };
}

#endif
