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
#ifndef RESTSource_hpp_
#define RESTSource_hpp_

#include <iostream>
#include <fstream>
#include <memory>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "SimpleWeb/server_http.hpp"

namespace pfabric {

  class RESTSource : public DataSource<TStringPtr> {
  public:
    enum RESTMethod {
      GET_METHOD = 0,
      POST_METHOD = 1,
      PUT_METHOD = 2,
      DELETE_METHOD = 3
    };

    PFABRIC_SOURCE_TYPEDEFS(TStringPtr);

    RESTSource(unsigned int port, const std::string& path, RESTMethod method, unsigned short numThreads = 1);

    /**
     * Deallocates all resources.
     */
    ~RESTSource();

    unsigned long start();
    void stop();

  protected:
    typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

    static std::shared_ptr<HttpServer> server;
    static std::once_flag onlyOne;

    void createHttpServer(unsigned int port, unsigned short numThreads = 1);
    void addRessource(const std::string& path, RESTMethod method);

    void produceTuple(const StringRef& data);
    void producePunctuation(PunctuationPtr pp);


  };

}

#endif
