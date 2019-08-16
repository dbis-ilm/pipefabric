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


#ifndef _LoopEdge_hpp
#define _LoopEdge_hpp

#include "NFAEdge.hpp"

namespace pfabric {
/**
 * @brief  an object represents a loop edge in our engine. the system will stay in this
 * loop until some condition should be satisfied such as reaching the limit of
 * allowed loop
 */
template<class Tin, class Tout, class Tdep>
class LoopEdge;
template<class Tin, class Tout, class Tdep>
class LoopEdge: public NFAEdge<Tin, Tout, Tdep> {

private:
	/**
	 * specify a limit for the number of loop
	 */
	short numOfLoop;
public:
	typedef ns_types::SharedPtr<LoopEdge<Tin, Tout, Tdep>> LoopEdgePtr;
	/**
	 * default constructor to set the edge id and its predicate
	 * @param NFAEdge_id the ID assigned to this edge
	 * @param predicate a condition to check by this edge to stay in
	 */
	LoopEdge(int edgeID, typename NFAEdge<Tin, Tout, Tdep>::EdgePredicate predicate) :
			NFAEdge<Tin, Tout, Tdep>(edgeID, predicate) {
		numOfLoop = std::numeric_limits<short>::max();
	}
	/**
	 * virtual destructor
	 */
	virtual ~LoopEdge() {}
	/**
	 * output member variable information.
	 * @param out the output stream handle.
	 */
	virtual void write(ostream& out = cout) const {
		out << "This is a loop edge with ID(" << this->edgeID << ") with loop number "
				<< numOfLoop;
	}
	/**
	 * equality comparison operator.
	 * @param rhs The input edge for this edge to compare to
	 * @return True iff this edge has the same predicate as that of rhs
	 */
	bool operator ==(const LoopEdge& rhs) const {
		return NFAEdge<Tin, Tout, Tdep>::operator ==(rhs) && numOfLoop == rhs.getNumOfLoop();
	}

	/**
	 * inequality comparison operator.
	 * @param rhs The input edge for this edge to compare to
	 * @return True iff this edge does not have the same predicate as that of rhs
	 */
	bool operator !=(const LoopEdge& rhs) const {
		return !(this->operator ==(rhs));
	}

	/**
	 * get the edge type (forward edge)
	 * @return as above
	 */
	const typename NFAEdge<Tin, Tout, Tdep>::EdgeType getEdgeType() const {
		return NFAEdge<Tin, Tout, Tdep>::Loop;
	}
	/**
	 * get the number of loops for this edge
	 * @return as above
	 */
	int getNumOfLoop() const {
		return numOfLoop;
	}
	/**
	 * set the number of loops for this edge
	 * @param  num_loop the number of loops for this edge to set
	 */
	void setNumOfLoop(int numLoop) {
		assert(numLoop >= 0);
		numOfLoop = numLoop;
	}

};
//class LoopEdge
}//namespace pfabric

#endif //_LoopEdge_hpp
