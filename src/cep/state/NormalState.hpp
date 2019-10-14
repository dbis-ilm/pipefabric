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

#ifndef NormalState_hpp_
#define NormalState_hpp_

#include <cassert>
#include <vector>
#include "NFAState.hpp"
#include "../edge/ForwardEdge.hpp"

namespace pfabric {

/**
 * @brief A class represents the normal state object in which the CEP can traverse through
 * the NFA. This state must have forward edges to jump to next state. Hence, the engine cam
 * go from state to another by traversing the edges to reach the final state.
 */
template <class Tin, class Tout, class Tdep>
class NormalState: public NFAState<Tin> {

private:
	/**
	 * This vector stores the forward edges of this state. By these edges, the engines
	 * can go from state to another state.
	 */
	std::vector<typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr> forwardEdges;
public:
	typedef ns_types::SharedPtr<NormalState<Tin, Tout, Tdep>> NormalStatePtr;
	/**
	 * A constructor to set the state id which has a type "normal"
	 * @param  stateID the state id to set
	 */
	NormalState(int stateID): NFAState<Tin>(stateID) {}
	/**
	 * A constructor to set the state id and its name
	 * @param  stateID the state id to set
	 * @param name the name of the state
	 *
	 */
	NormalState(int stateID, std::string name): NFAState<Tin>(stateID, name) {}
	/**
	 * A constructor to set the state id, name and forward edges for this state
	 * @param stateID the state id to set
	 * @param name the name of the state
	 * @param forwardEdges forward edges from this state
	 */
	NormalState(int stateID, std::string name,
			std::vector<typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr> forwardEdges): NFAState<Tin>(stateID, name), forwardEdges(forwardEdges) {}
	/**
	 * Get the state type (it should be here a normal state)
	 * @return as above
	 */
	virtual const typename NFAState<Tin>::StateType getStateType() const { return NFAState<Tin>::Normal; }
	/**
	 * Add an out-going edge associated to this  state.
	 * @param edge pointer to the edge to be added to this state
	 */
	void addEdge(typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr edge) {
		assert(edge);
		forwardEdges.push_back(edge);
	}

	/**
	 * Add an out-going edge to this state by specifying an edge id and a predicate
	 * associated with this edge
	 * @param id the id of that edge
	 * @param predicate the predicate of this edge
	 */
	void addEdge(int id, typename NFAEdge<Tin, Tout, Tdep>::EdgePredicate predicate) {
		assert(predicate);
		typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr forward(new ForwardEdge<Tin, Tout, Tdep>(id, predicate));
		forwardEdges.push_back(forward);
	}
	/**
	 * Get the number of edges for this state
	 * @return As above
	 */
	int getNumEdges() const { return this->forwardEdges.size(); }
	/**
	 * get all the forward edges that associated with this state
	 * @return as above
	 */
	const std::vector<typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr>& getForwardEdges() const { return this->forwardEdges; }
	/**
	 * set all the forward edges at once for this state
	 * @param forwardEdges the forward edges
	 */
	void setForwardEdges(std::vector<typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr> forwardEdges) { this->forwardEdges = forwardEdges; }
	/**
	 * Fetch a particular edge pointer from the edge collections given its index
	 * @return as above
	 */
	ForwardEdge<Tin, Tout, Tdep>* getForwardEdgeByIndex(int index) const {
		assert((std::size_t)index < forwardEdges.size());
		return forwardEdges.at(index).get();
	}
	/**
	 * Fetch a particular edge pointer from the edge collections given its id
	 * @return as above
	 */
	ForwardEdge<Tin, Tout, Tdep>* getForwardEdgeByID(int id) {
		for (int i = 0; i < forwardEdges.size(); i++)
				if (forwardEdges[i]->getID() == id)
					return forwardEdges[i].get();
			return NULL;
	}
	/**
	 * virtual destructor
	 */
	virtual ~NormalState() {}
	/**
	 * Equality comparison operator.
	 * @param rhs the input state for this state to compare to
	 * @return true iff this state has the same state id  and name as that of rhs
	 */
	bool operator ==(const NormalState& rhs) const {
		for (int i = 0; i < forwardEdges.size(); i++)
				if (ForwardEdgePtr(rhs.getForwardEdgeByIndex(i)) != forwardEdges[i])
					return false;
		return NFAState<Tin>::operator ==(rhs);
	}
	/**
	 * Inequality comparison operator.
	 * @param rhs The input state for this state to compare to
	 * @return true iff this state does not have the same state id  and name as that of rhs
	 */
	bool operator !=(const NormalState& rhs) const { return !(this->operator ==(rhs)); }
};
} /* namespace pfabric */
#endif /* NormalState_hpp_ */
