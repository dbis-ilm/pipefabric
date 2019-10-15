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


#ifndef StartState_hpp_
#define StartState_hpp_

#include "NormalState.hpp"

namespace pfabric {
/**
 *@brief A class represents the start state object in which the CEP can start detecting
 *@brief the complex event by the matcher operator and using the NFA controller
 */
template <class Tin, class Tout, class Tdep>
class StartState: public NormalState<Tin, Tout, Tdep> {

public:
	typedef ns_types::SharedPtr<StartState<Tin, Tout, Tdep>> StartStatePtr;
	/**
	 * constructor to set the state id
	 * @param stateID the state id to set
	 */
	StartState(int stateID): NormalState<Tin, Tout, Tdep>(stateID) {}
	/**
	 * constructor to set the state id and its name
	 * @param  state_id the state id to set
	 * @param name the name of the state
	 *
	 */
	StartState(int stateID, std::string name): NormalState<Tin, Tout, Tdep>(stateID, name) {}
	/**
	 * constructor to set the state id, name and forward edges for this state
	 * @param state_id the state id to set
	 * @param name the name of the state
	 * @param forward_edges forward edges from this state
	 */
	StartState(int stateID, std::string name,
			std::vector<typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr> forwardEdges): NormalState<Tin, Tout, Tdep>(stateID, name, forwardEdges) {}
	/**
	 * get the type of this state (start state).
	 * @return as above.
	 */
	const typename NFAState<Tin>::StateType getStateType() const { return NFAState<Tin>::Start; }
	/**
	 * virtual destructor
	 */
	virtual ~StartState() {}
};
} /* namespace pfabric */
#endif /* StartState_hpp_ */
