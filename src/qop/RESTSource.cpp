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

#include "SimpleWeb/server_http.hpp"
#include "RESTSource.hpp"
#include "core/Tuple.hpp"

using namespace pfabric;

std::once_flag RESTSource::onlyOne;
std::shared_ptr<RESTSource::HttpServer> RESTSource::server = nullptr;

RESTSource::RESTSource(unsigned int port, const std::string& path, RESTMethod method, unsigned short numThreads) {
  createHttpServer(port, numThreads);
  addRessource(path, method);
}

RESTSource::~RESTSource() {
}

unsigned long RESTSource::start() {
  BOOST_ASSERT(server.get() != nullptr);
  server->start();
  return 0;
}

void RESTSource::stop() {
  server->stop();
}

void RESTSource::createHttpServer(unsigned int port, unsigned short numThreads) {
  // TODO: we should create one HTTPServer per port
  std::call_once( RESTSource::onlyOne,
                 [] (int p, unsigned short nt) {
                   RESTSource::server.reset(new HttpServer(p, nt));
                   std::cout << "RESTSource:: HTTPServer created for port " << p << std::endl;
                 }, port, numThreads);
}

void RESTSource::addRessource(const std::string& path, RESTMethod method) {
  BOOST_ASSERT(server.get() != nullptr);
  std::string methodString[] = { "GET", "POST", "PUT", "DELETE" };
  server->resource[path][methodString[method]] = [this](HttpServer::Response& response,
                                                        std::shared_ptr<HttpServer::Request> request) {
    //Retrieve string:
    auto content = request->content.string();
    produceTuple(StringRef(content.c_str(), content.size()));
    response << "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
  };
}

void RESTSource::produceTuple(const StringRef& data) {
  auto tn = makeTuplePtr(data);
  this->getOutputDataChannel().publish(tn, false);
}

void RESTSource::producePunctuation(PunctuationPtr pp) {
  this->getOutputPunctuationChannel().publish(pp);
}
