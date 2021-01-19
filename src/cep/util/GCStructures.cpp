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

#include "GCStructures.hpp"
using namespace pfabric;

template<class Tin, class Tout>
GCStructures<Tin, Tout>::GCStructures(StructurePool* pool,
		CEPEngine<Tin, Tout>::WindowStruct *win, boost::atomic<bool>& indicator) :
		interrupted(new bool(false)), the_thread(NULL), pool(pool), win(win), par(
				NULL), cgIndicator(indicator) {
	the_thread = new boost::thread(&GCStructures::start, this);
}

template<class Tin, class Tout>
void GCStructures<Tin, Tout>::start() {
	if (win->window != CEPEngine::WindowStruct::NoConstraint)
		while (!*interrupted) {
			{
				//boost::unique_lock<boost::mutex> lock(q_mtx);
				while (cgIndicator == false) {
				}
				//condition.wait(lock);
				//std::cout << "wait done ... gc" << std::endl;
			}
			for (ValueIDMultimap<NFAStructurePtr>::MultimapConstIterator it =
					pool->beginConstIterator();
					it != this->pool->endConstIterator(); it++) {
				if (tuple->timestamp() - it->second->getFirstEventTimestamp()
						> win->period) {
					if (it->first != par) {
						//std::cout << "delete" << std::endl;
						pool->removeValue(it);
					}
				}
			}
			//boost::unique_lock<boost::mutex> lock(q_mtx);
			cgIndicator = false;
			//condition2.notify_one();
			//std::cout << "signal done ... gc" << std::endl;
		}

}
GCStructures::~GCStructures() {
	if (the_thread) {
		*interrupted = true;
		the_thread->join();
		delete the_thread;
	}
}

tuple_ptr GCStructures::getTuple() const {
	return tuple;
}

void GCStructures::setParameters(tuple_ptr tuple, pfabric::Partition* par) {
	this->tuple = tuple;
	this->par = par;
}

pfabric::Partition* GCStructures::getPartition() const {
	return par;
}
