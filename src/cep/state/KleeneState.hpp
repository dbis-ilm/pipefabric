/*
 * Copyright (c) 2014 The PipeFabric team,
 *                    All Rights Reserved.
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
#ifndef KleeneState_hpp_
#define KleeneState_hpp_

#include "NormalState.hpp"
#include "../edge/LoopEdge.hpp"
#include "../edge/ForwardEdge.hpp"
namespace pfabric {
/**
 * @brief a class represents the kleene state object. This kind of state has a loop edge
 * to stay in the same state unless a condition is satisfied to jump to the next state.
 * This condition can be
 */
template <class Tin, class Tout, class Tdep>
class KleeneState: public NormalState<Tin, Tout, Tdep> {

public:
	typedef  ns_types::SharedPtr<KleeneState<Tin, Tout, Tdep>> KleeneStatePtr;
	/**
	 * Star: zero or more
	 * Plus: one or more
	 * Question: at most one
	 * Restricted: a particular iteration number
	 */
	enum KleeneSpecification {
		Star, Plus, Question, Restricted
	};
	/**
	 * A constructor to set the state id, name , forward edges for this state and the type
	 * of kleene state
	 * @param stateID the state id to set
	 * @param name the name of the state
	 * @param forwardEdges forward edges from this state
	 * @param spec the type of this kleene state
	 */
	KleeneState(int stateID, string name,
			std::vector<typename ForwardEdge<Tin, Tout, Tdep>::ForwardEdgePtr> forwardEdges,  KleeneSpecification spec = Star) : NormalState<Tin, Tout, Tdep>(stateID, name, forwardEdges) {this->spec = spec; }
	/**
	 * constructor to set the state id and its specification
	 * @param  stateID the state id to set
	 * @param spec the type of this kleene state
	 */
	KleeneState(int stateID, KleeneSpecification spec =Star): NormalState<Tin, Tout, Tdep>(stateID) {this->spec = spec; };

	/**
	 * A constructor to set the state id, its name and the type of kleene state
	 * @param  stateID the state id to set
	 * @param name the name of the state
	 * @param spec the type of this kleene state
	 */
	KleeneState(int stateID, string name, KleeneSpecification spec=Star): NormalState<Tin, Tout, Tdep>(stateID, name) {
		this->spec = spec;
	}
	/**
	 * A constructor to set the state id, name, loop edge predicate and its specification
	 * @param  stateID the state id to set
	 * @param kleene_id the id of the loop edge
	 * param predicate a predicate associated with the edge
	 * @param spec the type of this kleene state
	 */
	KleeneState(int stateID, int kleeneID, typename NFAEdge<Tin, Tout, Tdep>::EdgePredicate predicate, KleeneSpecification spec=Star): NormalState<Tin, Tout, Tdep>(stateID), loop(new LoopEdge<Tin, Tout, Tdep>(kleeneID, predicate)) {
		this->spec = spec;
		assert(predicate);
	}
	/**
	 * Get the type of this state (kleene state).
	 * @return As above.
	 */
	const typename NFAState<Tin>::StateType getStateType() const { return NFAState<Tin>::Kleene; }
	/**
	 * virtual destructor
	 */
	virtual ~KleeneState() {}
	/**
	 * get the loop edge for this state
	 * @return as above
	 */
	LoopEdge<Tin, Tout, Tdep>* getLoopEdge() const { return loop; }
	/**
	 * set the loop edge for this state
	 * @param LoopEdge a loop edge to set for this state
	 */
	void setLoopEdge(LoopEdge<Tin, Tout, Tdep>* LoopEdge) { assert(LoopEdge); this->loop = LoopEdge; }
	/**
	 * equality comparison operator.
	 * @param rhs the input state for this state to compare to
	 * @return true iff this state has the same state id, name and specification as that of rhs
	 */
	bool operator ==(const KleeneState& rhs) const {
		return NFAState<Tin>::operator ==(rhs) && rhs.getLoopEdge() == getLoopEdge()
					&& rhs.getSpecification() == getSpecification();
	}
	/**
	 * An inequality comparison operator.
	 * @param rhs The input state for this state to compare to
	 * @return true iff this state does not have the same state id, name and specification as that of rhs
	 */
	bool operator !=(const KleeneState& rhs) const { return !(this->operator ==(rhs)); }
	/**
	 * Get the specification of this state whether start, plus, question or restricted
	 * @return as above
	 */
	const KleeneSpecification getSpecification() const { return this->spec; }
private:
	/**
	 * This kind of state is characterized by a loop edge to let CEP to stay within.
	 */
	LoopEdge<Tin, Tout, Tdep>* loop;
	/**
	 * Store the specification of this state whether start, plus, question or restricted
	 */
	KleeneSpecification spec;
};
} /* namespace pfabric */
#endif /* KleeneState_hpp_ */
