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

#ifndef ELEMENTSERIALIZABLE_HPP_
#define ELEMENTSERIALIZABLE_HPP_

#include "PFabricTypes.hpp"
#include "serialize.hpp"

namespace pfabric {
class ElementSerializable {
public:
	virtual void deserializeFromStream(StreamType& res) = 0;
	virtual void serializeToStream(StreamType& res) const = 0;
	virtual ~ElementSerializable() {} // to remove compiler warning
};
}
#endif /* ELEMENTSERIALIZABLE_HPP_ */
