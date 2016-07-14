/*
 * zmq_source.cpp
 *
 *  Created on: 28 May 2013
 *      Author: Omran Saleh <omran.saleh@tu-ilmenau.de>
 */

#include "ZMQSource.hpp"

#include <cassert>

using namespace pfabric;


ZMQSourceImpl::ZMQSourceImpl(const std::string& path,
															ZMQParams::SourceType stype,
															ZMQParams::EncodingMode emode,
			    										TStringCallbackFunc const& cb1,
															BufCallbackFunc const& cb2,
															PunctuationCallbackFunc const& cb3 ) :
	mTStringCB(cb1), mBufCB(cb2), mPunctuationCB(cb3),
	mNumTuples(0), mMode(emode), mInterrupted(true), /*mSourceThread(nullptr),*/ mType(stype) {

  switch (mType) {
  case ZMQParams::SubscriberSource:
    mSocket = std::make_unique< sock::ZMQSocket >(path, ZMQ_SUB);
    break;
  case ZMQParams::PullSource:
    mSocket = std::make_unique< sock::ZMQSocket >(path, ZMQ_PULL);
    break;
  default:
    BOOST_ASSERT_MSG(false, "unsupported source type for ZMQSource");
  }
  start();
}

unsigned long ZMQSourceImpl::process() {
  mInterrupted = false;

  StringRef result;
  while (mInterrupted == false) {
    try {
    if (mMode == ZMQParams::AsciiMode) {
      auto res = mSocket->recvString(result);
      if (res > 0) {
        mTStringCB(makeTuplePtr(result));
      }
    }
    else {
 				zmq::message_t& msg = mSocket->recvMessage();
				if (msg.size() != 0) {
	  			uint8_t *ptr = (uint8_t *) msg.data();
	  			StreamType buf;
	  			buf.assign(ptr, ptr + msg.size());
					auto tp = makeTuplePtr((const StreamType&) buf);
	  			mBufCB(tp);
				}
      }
    }
    catch (sock::ZMQSocketException& exc) {
    }
    /*catch (zmq::error_t & exp) {
      std::cout << "error : " << exp.what() << std::endl;
      break;
    }*/

  }
  mPunctuationCB(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
  return mNumTuples;
}


ZMQSourceImpl::~ZMQSourceImpl() {
  if (!mInterrupted) {
    stop();
  }
}

void ZMQSourceImpl::start() {
  // synchronize this method to block until the thread is started
  std::lock_guard<std::mutex> lock_gd(mZmqMtx);

  if (mInterrupted == true) {
    std::unique_lock<std::mutex> sync_lock(mStartMtx);
    
    mSourceThread = std::thread(&ZMQSourceImpl::process, this);
  }
}

void ZMQSourceImpl::stop() {
  // synchronize this method to block until the thread is stopped
  std::lock_guard<std::mutex> lock_gd(mZmqMtx);

  if (mInterrupted == false) {
    mInterrupted = true;
    try {
      if (mSourceThread.joinable())
       mSourceThread.join();
    }
    catch (sock::ZMQSocketException& exc) {
    }
  }
}

bool ZMQSourceImpl::isInterrupted() const {
  return mInterrupted;
}

/*
void ZMQSourceImpl::sync() {
	std::string test;
	std::string path = this->mSocket->getSocketPath();
	std::string::size_type pos = path.find_last_of(':');
	int port = atoi(path.substr(pos + 1).c_str());
	port += 1;
	std::string sync_address = path.substr(0, pos);
	std::stringstream sync_path;
	sync_path << sync_address << ":"<< port;
	std::cout << "try to sync in zmq_source: " << sync_path.str() << std::endl;
	sock::ZMQSocket* the_socket = new sock::ZMQSocket(sync_path.str(), ZMQ_REQ);
	the_socket->sendString("");
	the_socket->recvString(test);
	delete the_socket;
}
*/
