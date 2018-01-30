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

#ifndef GCStructures_hpp_
#define GCStructures_hpp_
#include "../StructurePool.hpp"
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include "ValueIDMultimap.hpp"
#include "Partition.hpp"
#include "../CEPEngine.hpp"
namespace pfabric {
template<class Tin, class Tout>
class GCStructures {
public:
	GCStructures(StructurePool* pool, CEPEngine<Tin, Tout>::WindowStruct *win1, boost::atomic<bool> &cg );
	virtual ~GCStructures();
	tuple_ptr getTuple() const;
	void setParameters(tuple_ptr tuple, pfabric::Partition* par);
	pfabric::Partition* getPartition() const;
	void start();
private:
	boost::thread* the_thread;
	boost::shared_ptr<bool> interrupted;
	structure_pool* pool;
	tuple_ptr tuple ;
	pfabric::Partition* par;
	CEPEngine<Tin, Tout>::WindowStruct *win;
	boost::atomic<bool>& cgIndicator ;
};
}
#endif /* GCStructures_hpp_ */
