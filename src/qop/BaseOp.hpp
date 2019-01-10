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

#ifndef BaseOp_hpp_
#define BaseOp_hpp_

#include <string>

namespace pfabric {

/**
 * @brief BaseOp represents a common base class for all parameterizable query operators.
 *
 * The only purpose of this class is to provide a common interface for query operators
 * which allow to change parameters at runtime.
 */
class BaseOp {
public:

	virtual ~BaseOp() {}

	virtual const std::string opName() const { return std::string("BaseOp"); }
};

} // namespace pfabric

#endif
