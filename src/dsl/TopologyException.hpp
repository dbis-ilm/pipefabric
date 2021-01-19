/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef TopologyException_hpp_
#define TopologyException_hpp_

#include <string>
#include <exception>

#include "fmt/format.h"

class TopologyException : public std::exception {
  std::string msg;

public:
    TopologyException(const char *s = "") : msg(s) {}

    virtual const char* what() const throw() {
      return fmt::format("TopologyException: {}", msg).c_str();
    }
};

#endif
