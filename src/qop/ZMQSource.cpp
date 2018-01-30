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
