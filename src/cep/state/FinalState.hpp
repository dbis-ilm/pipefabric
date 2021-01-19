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



#ifndef FinalState_hpp_
#define FinalState_hpp_

#include "NFAState.hpp"

namespace pfabric {
/**
 * @brief a class represents the final state object in which the CEP can detect the complex
 * event once it reaches this state type. After that, the system produce the results immediately
 */
template <class Tin, class Tout, class Tdep>
class FinalState: public NFAState<Tin> {

public:
	typedef ns_types::SharedPtr<FinalState<Tin, Tout, Tdep>> FinalStatePtr;
	/**
	 * A constructor to set the state id
	 * @param stateID the state id to set
	 */
	FinalState(int stateID): NFAState<Tin>(stateID) { }
	/**
	 * A constructor to set the state id and its name
	 * @param  stateID the state id to set
	 * @param name the name of the state
	 */
	FinalState(int stateID, std::string name): NFAState<Tin>(stateID, name) {}
	/**
	 * Get the type of this state (final).
	 * @return as above.
	 */
	const typename NFAState<Tin>::StateType getStateType() const { return NFAState<Tin>::Final; }
	/**
	 * virtual destructor
	 * Nothing to do here
	 */
	virtual ~FinalState() {}
};
} /* namespace pfabric */
#endif /* FinalState_hpp_ */
