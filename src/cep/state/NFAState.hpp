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



#ifndef NFAState_hpp
#define NFAState_hpp

#include <iostream>
#include <boost/shared_ptr.hpp>

namespace pfabric {
/**
 * @brief A NFA state object represents an automaton state in our engine. This state can
 * can be either start, kleene (repetition), final, normal or negated states. Every state has its own
 * behavior to detect a part from a complex event.
 */
template <class Tin>
class NFAState {

public:
	typedef ns_types::SharedPtr<NFAState <Tin> > StatePtr;
	/**
	 * Enumeration represents the type of the state either start, kleene, final
	 * normal or negated states which is used to know how the engine should process or store the
	 * incoming tuples. Each incoming tuple should pass some of these states during the detection
	 * of the complex event.
	 */
	enum StateType {
		Start, Kleene, Final, Normal, Negation
	};
	/**
	 * A constructor to set the state ID (an integer value) to distingush a state from another
	 * during the traversal of tuples in the NFA.
	 * @param stateID The state ID to set
	 */
	NFAState(int stateID) : stateID(stateID) {}

	/**
	 * A constructor to set the state ID and its name. The name is a unique identifier for
	 * this state which is necessary to present the state in the output.
	 * @param stateID The state ID to set
	 * @param name the name of the state to set
	 */
	NFAState(int stateID, std::string name): stateID(stateID), stateName(name) {}
	/**
	 * Virtual destructor to release the resource
	 * Nothing to do here
	 */
	virtual ~NFAState() {}

	/**
	 * Output member variable information
	 * @param out The output stream handle.
	 */
	virtual void write(std::ostream& out = std::cout) const {
		out << "Name of this state: " << stateName << std::endl;
		out << " and id = " << stateID << std::endl;
	}
	/**
	 * Get the type of this state either start, kleene, final normal or negated states
	 * represented as integer.
	 * @return as above.
	 */
	const virtual StateType getStateType() const = 0;
	/**
	 * Get the id of this state
	 * @return as above
	 */
	int getStateID() const { return this->stateID; }
	/**
	 * Set the state id after creating the state
	 * @pram state_id the state id
	 */
	void setStateID(int stateID) { this->stateID = stateID; }
	/**
	 * Get the state name as string
	 * @return as above
	 */
	std::string getStateName() const { return this->stateName; }
	/**
	 * Set the state name as string
	 * @param stateName the name of this state ro be shown
	 */
	void setStateName(std::string stateName) { this->stateName = stateName; }

	/**
	 * Equality comparison operator.
	 * @param rhs the input state for this state to compare to
	 * @return true iff this state has the same state id and name as that of rhs
	 */
	virtual bool operator ==(const NFAState<Tin>& rhs) const {
		if (this == &rhs)
			return true;
		return stateID == rhs.getStateID() && stateName == rhs.getStateName();
	}
	/**
	 * Inequality comparison operator.
	 * @param rhs The input state for this state to compare to
	 * @return true iff this state does not have the same state id  and name as that of rhs
	 */
	virtual bool operator !=(const NFAState<Tin>& rhs) const { 	return !(this->operator ==(rhs)); }

protected:
	/**
	 * This variable stores the ID of this state assigned by our engine.
	 */
	int stateID;
	/**
	 * The name of the state
	 */
	std::string stateName;

};
//class NFAState
} //namespace pfabric

#endif //_NFAState_hpp
