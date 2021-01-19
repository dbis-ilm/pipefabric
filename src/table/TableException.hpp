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

#ifndef TableException_hpp_
#define TableException_hpp_

#include <exception>
#include <iostream>

#include "fmt/format.h"

namespace pfabric {

/**
 * @brief An exception for signaling errors in table processing.
 *
 * TableExecption is an exception class for signaling errors while
 * processing a table.
 */
class TableException : public std::exception {
  std::string msg;  //< a message string

 public:
  /**
   * Construct a new TableException instance.
   *
   * @param s the message string
   */
  TableException(const char* s = "") : msg(s) {}

  /**
   * Returns th message string describing the exception.
   *
   * @return the message string
   */
  virtual const char* what() const throw() {
    return fmt::format("TableException: {}", msg).c_str();
  }
};

}

#endif

