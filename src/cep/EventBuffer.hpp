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

#ifndef EventBuffer_hpp_
#define EventBuffer_hpp_
#include "Instance.hpp"
#include "util/valueIDMap.hpp"
namespace pfabric {
/**
 * A container or an event buffer to store all matching events
 */
template<class Tin, class Tout>
class EventBuffer: public ValueIDMap<typename Instance<Tin>::InstancePtr> {

public:

	typedef boost::shared_ptr<EventBuffer<Tin, Tout>> EventBufferPtr;
	/**
	 * a default constructor
	 */
	EventBuffer() {}

	/**
	 * A virtual destructor
	 */

	virtual ~EventBuffer() {}
	/**
	 * Output the information  into ostream object
	 * @param out The output stream handle.
	 */
	void print(ostream& out = cout) const {
		/*
		 os << std::endl;
		 for (ValueIDMap<InstancePtr>::MapConstIterator it = this->valueID.begin();
		 it != this->valueID.end(); it++) {
		 os << " [" << it->first << ", ";
		 it->second->print(os);
		 os << "]";
		 }
		 os << std::endl;*/
	}

	/**
	 * Get the event of a given ID.
	 * @param id The ID given to find the event
	 * @return The event of the given ID
	 */
	typename Instance<Tin>::InstancePtr getEvent(const long id) const {
		ValueIDMap<typename Instance<Tin>::InstancePtr>::getValue(id);
		return typename Instance<Tin>::InstancePtr();
	}
};
}
#endif /* EventBuffer_hpp_ */
