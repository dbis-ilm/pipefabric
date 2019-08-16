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

 
#include <iostream>
#include "ZMQSocket.hpp"

#define ZERO_COPY

using namespace pfabric;
using namespace pfabric::sock;

ZMQSocket::ZMQSocket(const std::string& path, int type, short value, size_t len,
		const std::string& name) :
		mSocketName(name), mSocketPath(path), mSocketType(type), value(value) {
	mCtxPtr = new zmq::context_t(1);
	configureSocket(len);
}

const std::string& ZMQSocket::getSocketName() const {
	return mSocketName;
}

void ZMQSocket::setSocketName(const std::string& gate_name) {
	this->mSocketName = gate_name;
}

const std::string& ZMQSocket::getSocketPath() const {
	return mSocketPath;
}

void ZMQSocket::setSocketPath(const std::string& socket_path) {
	this->mSocketPath = socket_path;
}

int ZMQSocket::getSocketType() const {
	return mSocketType;
}

void ZMQSocket::setSocketType(int socket_type) {
	this->mSocketType = socket_type;
}

ZMQSocket::~ZMQSocket() {
  if (mZMQSockPtr)
    delete mZMQSockPtr;
  if (mCtxPtr)
    delete mCtxPtr;
}

void ZMQSocket::configureSocket(size_t len) {
	mZMQSockPtr = new zmq::socket_t(*mCtxPtr, mSocketType);
	const char *path = mSocketPath.c_str();
	switch (mSocketType) {
	case ZMQ_PULL:
	case ZMQ_PUB:
	case ZMQ_REP:
		mZMQSockPtr->setsockopt(ZMQ_SNDHWM, &value, sizeof(int));
		mZMQSockPtr->bind(path);
		break;
	case ZMQ_SUB:
		//mZMQSockPtr->setsockopt(ZMQ_RCVHWM, &value, sizeof(int));
		mZMQSockPtr->setsockopt(ZMQ_SUBSCRIBE, &value, len);
	case ZMQ_PUSH:
	case ZMQ_REQ:
		mZMQSockPtr->connect(path);
	}
  int timeout = 2000;
  mZMQSockPtr->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
  int linger = 0;
  mZMQSockPtr->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
}

void* ZMQSocket::getContext() {
	return (void *) (mCtxPtr);
}

bool ZMQSocket::sendString(const std::string& str) {
#ifndef ZERO_COPY
	/**
	 * This initialisation will generate duplicated messages in the new versions
	 */
	zmq::message_t msg((void *) str.c_str(), str.length(), NULL); //
#else
	/**
	 * the best solution to prevent duplicated messages
	 */
	zmq::message_t msg(str.size());
	memcpy(msg.data(), str.data(), str.size());
#endif
	//nanosleep(&tim, NULL);
	//message.rebuild((void *) str.c_str(), str.length(), NULL); this also will generate duplicated messages in the new versions
	return mZMQSockPtr->send(msg);
}

bool ZMQSocket::sendBuffer(const std::vector<uint8_t>& buf) {
#ifndef ZERO_COPY
	zmq::message_t msg((void *) buf.data(), buf.size(), NULL);
#else
	zmq::message_t msg(buf.size());
	memcpy(msg.data(), buf.data(), buf.size());
#endif
	return mZMQSockPtr->send(msg);
}

bool ZMQSocket::sendBuffer(char *buf, int len) {
	/**
	 * Here the same
	 */
#ifndef ZERO_COPY
	zmq::message_t msg((void *) buf, len, NULL);
#else
	zmq::message_t msg(len);
	memcpy(msg.data(), buf, len);
#endif
	//message.rebuild((void *) buf, len, NULL);
	return mZMQSockPtr->send(msg);
}

int ZMQSocket::recvString(StringRef& data, bool blocking) {

	int retval = -1;
	int flags = (blocking == false) ? ZMQ_NOBLOCK : 0;

	if (mZMQSockPtr != NULL) {

		try {
			retval = mZMQSockPtr->recv(&message, flags);
		}
		catch (zmq::error_t & ex) {
      if(ex.num() != ETERM)
        throw;
      return -1;
    }
		if (retval != 1) {
			retval = -1;
			std::string error("Failed to receive zeromq message: ");
			error.append(zmq_strerror(errno));
			throw ZMQSocketException(error);
		}
		retval = message.size();
		if (retval > 0) {
			// avoid to copy message string
			data.setValues(static_cast<char*>(message.data()), retval);
 		}
	}

	return retval;
}

int ZMQSocket::recvBuffer(char *buf, bool blocking) {
	int retval = -1;
	int flags = (blocking == false) ? ZMQ_NOBLOCK : 0;
	if (mZMQSockPtr != NULL) {
		try {
			retval = mZMQSockPtr->recv(&message, flags);
		}
		catch (zmq::error_t & exp) {
			//std::cout << "error : " << exp.what() << std::endl;
			throw ;
		}
		if (retval != 1) {
			retval = -1;
			std::string error("Failed to receive zeromq message: ");
			error.append(zmq_strerror(errno));
			throw ZMQSocketException(error);
		}
		retval = message.size();
		if (retval > 0) {
			memcpy((void*) buf, message.data(), retval);
		}
	}
	return retval;
}


void ZMQSocket::closeSocket() {
	mZMQSockPtr->close();
}

zmq::message_t& ZMQSocket::recvMessage(bool blocking) {
		int retval = -1;
		int flags = (blocking == false) ? ZMQ_NOBLOCK : 0;

		if (mZMQSockPtr != NULL) {
			try {
				retval = mZMQSockPtr->recv(&message, flags);
			}
			catch (zmq::error_t & exp) {
				//std::cout << "error : " << exp.what() << std::endl;
				throw ;
			}
			if (retval != 1) {
				retval = -1;
				std::string error("Failed to receive zeromq message: ");
				error.append(zmq_strerror(errno));
				throw ZMQSocketException(error);
			}
		}

		return message;
}


void ZMQSocket::connect(const std::string& path) {
	assert(mSocketType == ZMQ_SUB);
	mZMQSockPtr->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	mZMQSockPtr->connect(path.c_str());
}
