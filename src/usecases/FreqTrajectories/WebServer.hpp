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
