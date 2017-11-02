//
//  WebServer.hpp
//  pipefabric
//
//  Created by Kai-Uwe Sattler on 05.09.17.
//
//

#ifndef WebServer_hpp_
#define WebServer_hpp_

#include "SimpleWeb/server_http.hpp"
#include "SimpleWeb/utility.hpp"

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

extern std::shared_ptr<std::thread> runWebServer(HttpServer& server, const std::string& webRoot);

#endif /* WebServer_hpp_ */
