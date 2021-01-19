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




#ifndef ForwardEdge_hpp
#define ForwardEdge_hpp

#include "NFAEdge.hpp"
#include "../state/NFAState.hpp"
namespace pfabric {
template<class Tin>
class NFAState;
/**
 * @brief an object represents a forward edge in our engine, the CEP uses this kind
 * of edge to jump from state to state
 */
template<class Tin, class Tout, class Tdep>
class ForwardEdge: public NFAEdge<Tin, Tout, Tdep> {
public:
	typedef ns_types::SharedPtr<ForwardEdge<Tin ,Tout, Tdep>> ForwardEdgePtr;
	/**
	 * default constructor to set the ID of the edge
	 * @param edge_id the edge ID assigned to this edge
	 */
	ForwardEdge(int edgeID): NFAEdge<Tin, Tout, Tdep>(edgeID) {}
	/**
	 * default constructor to set the ID of the edge and its predicate
	 * @param edge_id the edge ID assigned to this edge
	 * @param predicate the predicate assigned to this edge
	 */
	ForwardEdge(int edgeID, typename NFAEdge<Tin, Tout, Tdep>::EdgePredicate predicate): NFAEdge<Tin, Tout, Tdep>(edgeID, predicate) {}
	/**
	 * virtual destructor
	 */
	virtual ~ForwardEdge() {}
	/**
	 * output member variable information.
	 * @param out the output stream handle.
	 */
	virtual void write(std::ostream& out = std::cout) const {
		out << "This is a forward edge with ID(" << this->edgeID
					<< ") to destination state " << destState->getStateID();
	}
	/**
	 * set the destination state of this edge.
	 * @param dest_state the destination state to set to.
	 */
	void setDestState(NFAState<Tin>*  destState) { this->destState = destState; }

	/**
	 * equality comparison operator.
	 * @param rhs the input edge for this edge to compare to
	 * @return true iff this edge has the same predicate and destination  state as that of rhs
	 */
	 bool operator ==(const ForwardEdge& rhs) const { return NFAEdge<Tin, Tout, Tdep>::operator ==(rhs) && rhs.getDestState() == destState; }

	/**
	 * inequality comparison operator.
	 * @param rhs the input edge for this edge to compare to
	 * @return true iff this edge does not have the same predicate and
	 * destination state as that of rhs
	 */
	 bool operator !=(const ForwardEdge& rhs) const { return !(this->operator ==(rhs)); }
	/**
	 * get the edge type (forward edge)
	 * @return as above
	 */
	const typename NFAEdge<Tin, Tout, Tdep>::EdgeType getEdgeType() const { return NFAEdge<Tin, Tout, Tdep>::Forward; }
	/**
	 * get the destination state of this edge. It can be kleene, final and negated states
	 * @return the destination state to this edge.
	 */
	NFAState<Tin>* getDestState() const { return this->destState; }

private:
	/**
	 * this variable stores the destination state of this forward edge.
	 * it can be kleene, final and negated states
	 */
	NFAState<Tin>*  destState;
};
//class ForwardEdge;
}//namespace pfabric

#endif //ForwardEdge_hpp
