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

 
#ifndef zmq_socket_hpp_
#define zmq_socket_hpp_

#include <stdexcept>
#include <vector>

#include <zmq.hpp>

#include "core/parser/StringRef.hpp"

namespace pfabric {

	struct ZMQParams {
		/**
		 * Type of encoding of ZMQ messages.
		 */
		enum EncodingMode {
			BinaryMode,   //< binary encoding
			AsciiMode,    //< ascii (string) encoding
			MsgPackMode     //< ???
		};

		/**
		 *
		 */
		enum SinkType {
			PushSink,     //<
			PublisherSink       //<
		};

		/**
		 *
		 */
		enum SourceType {
			PullSource,     //<
			SubscriberSource       //<
		};
	};

namespace sock {
/**
 * Runtime exception for zeromq
 */
class ZMQSocketException: public std::runtime_error {
public:
	ZMQSocketException(const std::string& msg) :
			runtime_error(msg) {
	}
};

/*
 * Class to work with zeromq socket ..
 * 1. creating publisher, subscriber, requester and replier (publisher ZMQ_PUB, subscriber ZMQ_SUB,
 * requester ZMQ_REQ or replier ZMQ_REP)
 * 2. sending and receiving data by a socket
 */

class ZMQSocket {
private:
	/**
	 * the name of the socket (optional)
	 */
	std::string mSocketName;
	/**
	 * the path of the socket where the communication will be done
	 */
	std::string mSocketPath;
	/**
	 * the path of the socket where the synchronization will be done
	 */
	std::string mSocketSyncPath;
	/**
	 * socket type whether publisher ZMQ_PUB, subscriber ZMQ_SUB, requester ZMQ_REQ or replier ZMQ_REP
	 */
	int mSocketType;
	/**
	 * the created socket
	 */
	zmq::socket_t * mZMQSockPtr;
	/**
	 * the created sync socket (possibly nullptr)
	 */
	zmq::socket_t * mZMQSyncSockPtr;
	/**
	 * socket context
	 */
	zmq::context_t *mCtxPtr;
	/**
	 * The message arrived or received
	 */
	zmq::message_t message;


	short value;
public:
	/**
	 * constructor to create the socket according to its path and type
	 */
	ZMQSocket(const std::string& path, const std::string& synPath, int type, short value = '\0', size_t len = 0,
				const std::string& name = "");
	/**
	 * destructor
	 */
	virtual ~ZMQSocket();
	/**
	 * get the name of the socket
	 * @return the name of the socket
	 */
	const std::string& getSocketName() const;
	/**
	 * set the name of the socket
	 * @param gate_name the new name for this socket
	 */
	void setSocketName(const std::string& gate_name);

	/**
	 * get the socket path
	 * return the socket path
	 */
	const std::string& getSocketPath() const;
	/**
	 * set the socket path  where the communication will be done
	 * @param mSocketPath the socket path
	 */
	void setSocketPath(const std::string& socketPath);
	/**
	 * get the socket type whether publisher, subscriber, requester or replier
	 * @return the socket type
	 */
	int getSocketType() const;
	/**
	 * set the socket type whether publisher, subscriber, requester or replier
	 * @param socket_type the socket type
	 */
	void setSocketType(int socket_type);
	/**
	 * get socket context
	 */
	void *getContext();
	/**
	 * send a string by this socket
	 * @param string a string to be sent
	 */
  int sendString(const std::string &string);
	/**
	 * receive a string from this socket
	 * @param string a string to store the result
	 */
	int recvString(StringRef& data, bool blocking = true);
	/**
	 * send a buffer by this socket
	 * @param buf a buffer to be sent
	 * @param len the length of the buffer
	 */
	int sendBuffer(char *buf, int len);

  int sendBuffer(const std::vector<uint8_t>& buf);

	/**
	 * configure (create) the socket according to its path and type
	 */
	void configureSocket(size_t len);
	/**
	 * receive a buffer from this socket
	 * @param buf the buffer to store the data
	 */
	int recvBuffer(char *buf, bool blocking = true);

	/**
	 * receive a message from this socket
	 */
	zmq::message_t&  recvMessage( bool blocking = true);

	/**
	 * close the socket
	 */
	void closeSocket();

	void connect(const std::string& path);

};
}
}

#endif
