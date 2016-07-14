/*
 * zmq_source.hpp
 *
 *  Created on: 28 May 2013
 *      Author: Omran Saleh <omran.saleh@tu-ilmenau.de>
 */

#ifndef ZMQSource_hpp_
#define ZMQSource_hpp_

#include <string>
#include <vector>
#include <thread>

// #include <boost/thread.hpp>

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

typedef Tuple<StreamType> TBuf;
typedef TuplePtr<TBuf> TBufPtr;
/**
 * a zeromq source (subscriber) to receive stream tuples from a socket
 */
class ZMQSourceImpl {
public:
  typedef std::function< void(TStringPtr) > TStringCallbackFunc;
  typedef std::function< void(TBufPtr) > BufCallbackFunc;
  typedef std::function< void(PunctuationPtr) > PunctuationCallbackFunc;

	/**
	 * Constructor to assign a path for the socket, assign the schema and specify the type of encoding.
	 *
	 * @param path  the socket path  where AndunIn2 should receive the tuples
	 * @param schema the schema of the stream
	 * @param emode the type of encoding
	 */
  ZMQSourceImpl(const std::string& path,
    ZMQParams::SourceType stype,
    ZMQParams::EncodingMode emode,
		TStringCallbackFunc const& cb1,
    BufCallbackFunc const& cb2,
    PunctuationCallbackFunc const& cb3);

	/**
	 * Destructor : release resources.
	 */
	~ZMQSourceImpl();

	/**
	 * start the zmq source (start the thread)
	 */
	void start();
	/**
	 * stop the zmq source (start the thread)
	 */
	void stop();
	/**
	 * Interrupted or not
	 */
	bool isInterrupted() const;

	/**
	 * for synchronizing this source with a publisher
	 * the path of synchronization is the same address but the port is increased by one
	 */
//	void sync();

private:

	// void produceTuple(const std::string& sdata);

	/**
	 * try to receive and process incoming tuples
	 * @return number of received tuples
	 */
	unsigned long process();

 	TStringCallbackFunc mTStringCB;
	BufCallbackFunc mBufCB;
	PunctuationCallbackFunc mPunctuationCB;

	typedef std::unique_ptr< sock::ZMQSocket > ZMQSocketPtr;

	ZMQSocketPtr mSocket;                //< the subscriber socket
	int mNumTuples;                      //< number tuple processed by the socket
	ZMQParams::EncodingMode mMode;       //< the encoding mode
	bool mInterrupted;
  std::thread mSourceThread;             //< socket thread
	mutable std::mutex mZmqMtx;
	mutable std::mutex mStartMtx;
	// boost::condition_variable mStarted;
	ZMQParams::SourceType mType;
};

	template<typename Tout>
	class ZMQSourceBase : public DataSource<Tout> {
    PFABRIC_SOURCE_TYPEDEFS(Tout)

	public:
		/**
		 * Constructor to assign a path for the socket, assign the schema and specify the type of encoding.
		 *
		 * @param path  the socket path  where AndunIn2 should receive the tuples
		 * @param schema the schema of the stream
		 * @param emode the type of encoding
	 */
		ZMQSourceBase(const std::string& path,
              ZMQParams::SourceType stype = ZMQParams::SubscriberSource,
							 ZMQParams::EncodingMode emode = ZMQParams::BinaryMode,
               ZMQSourceImpl::TStringCallbackFunc const& cb1 = nullptr,
               ZMQSourceImpl::BufCallbackFunc const& cb2 = nullptr) :
			mImpl(new ZMQSourceImpl( path, stype, emode,
        cb1, cb2,
        std::bind(&ZMQSourceBase::publishPunctuation, this, std::placeholders::_1))) {}

		void stop() {
			mImpl->stop();
		}

    unsigned long start() {
      mImpl->start();
      return 0;
    }

	protected:

		void publishPunctuation(PunctuationPtr pp) {
			this->getOutputPunctuationChannel().publish(pp);
		}

	private:
		std::unique_ptr< ZMQSourceImpl > mImpl;
	};

  template<typename T>
  class ZMQSource : public ZMQSourceBase<T> {};

  template<>
  class ZMQSource<TBufPtr>  : public ZMQSourceBase<TBufPtr> {
    PFABRIC_SOURCE_TYPEDEFS(TBufPtr)

  public:
    ZMQSource(const std::string& path, ZMQParams::SourceType stype = ZMQParams::SubscriberSource) :
      ZMQSourceBase<TBufPtr>(path, stype, ZMQParams::BinaryMode,
        nullptr,
        std::bind(&ZMQSource<TBufPtr>::publishTuple, this, std::placeholders::_1)) {}

        void publishTuple(TBufPtr tp) {
    			this->getOutputDataChannel().publish(tp, false);
    		}


  };

  template<>
  class ZMQSource<TStringPtr> : public ZMQSourceBase<TStringPtr> {
    PFABRIC_SOURCE_TYPEDEFS(TStringPtr)

  public:
    ZMQSource(const std::string& path, ZMQParams::SourceType stype = ZMQParams::SubscriberSource) :
    ZMQSourceBase<TStringPtr>(path, stype, ZMQParams::AsciiMode,
      std::bind(&ZMQSource<TStringPtr>::publishTuple, this, std::placeholders::_1),
    nullptr) {}

      void publishTuple(TStringPtr tp) {
        this->getOutputDataChannel().publish(tp, false);
      }

  };
}

#endif
