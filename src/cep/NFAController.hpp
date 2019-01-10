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


#ifndef NFAController_hpp_
#define NFAController_hpp_

#include "state/NormalState.hpp"
#include "state/StartState.hpp"
#include "state/FinalState.hpp"
#include "state/KleeneState.hpp"
#include "state/NegationState.hpp"
#include "edge/NFAEdge.hpp"

#include <iostream>


namespace pfabric {
/*
 * A controller class to construct the NFA for detecting the complex event, the user
 * should create the states, edges and transitions by calling particular methods
 */
template<class TinPtr, class ToutPtr, class TdepPtr>
class NFAController {
private:
	/**
	 * a vector to store all states except final, kleene and negated states and start state
	 */
	std::vector<typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr> normalStates;
	/**
	 * a vector to store all kleene states
	 */
	std::vector<typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneStatePtr> kleeneStates;
	/**
	 * a vector to store all negated states
	 */
	std::vector<typename NegationState<TinPtr, ToutPtr, TdepPtr>::NegationStatePtr> negatedStates;
	/**
	 * the start state
	 */
	typename StartState<TinPtr, ToutPtr, TdepPtr>::StartStatePtr start;
	/**
	 * a vector to store the final states, in any NFA we have multiple states from this kind
	 */
	std::vector<typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr> finalStates;
	/**
	 * a vector to store the transitions between the states, each transition must contain an edge which has
	 * a predicate to evaluate
	 */
	std::vector<typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr> transitions;
	/**
	 * a state counter to assign a particular id for each state
	 */
	int stateCountID;
	/**
	 * an edge counter to assign a particular id for each edge
	 */
	int edgeCountID;
	/**
	 * get a pointer for a state with a given id
	 * @param id the id of the state
	 * @return as above
	 */
	typename NFAState<TinPtr>::StatePtr getState(int id);
	/**
	 * get a pointer for an edge  with a given id
	 * @param the id of the edge
	 * @return as above
	 */
	typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr getEdge(int id);


public:
	typedef std::shared_ptr<NFAController<TinPtr, ToutPtr, TdepPtr>> NFAControllerPtr;
	typedef std::function<TdepPtr()> initDependency;
	typedef std::function<void(const TdepPtr&, int, const TinPtr&  )> updateDependency;
	/**
	 * controller constructor, nothing to do
	 */
	NFAController() {
		edgeCountID = 0;
		stateCountID = 0;
		init = [&]() {
			return nullptr;
		};
		update = [&](const TdepPtr& tp, int id, const TinPtr& event ) {};
	}
	/**
	 * a destructor, nothing to do
	 */
	~NFAController() {
	}
	/**
	 * create the start state for this NFA and assign its name
	 * @param name the name of the start state
	 * @return a pointer to start state
	 */
	typename StartState<TinPtr, ToutPtr, TdepPtr>::StartStatePtr createStartState(string name) {
		start.reset(new StartState<TinPtr, ToutPtr, TdepPtr>(stateCountID++, name));
		return start;
	}
	/**
	 * create a normal state for this NFA and assign its name, this state should not
	 * be final state or start state or kleene state
	 * @param name the name of this normal state
	 * @return a pointer to a normal state
	 */
	typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr createNormalState(string name) {
		typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr state(
				new NormalState<TinPtr, ToutPtr, TdepPtr>(stateCountID++, name));
		normalStates.push_back(state);
		return state;
	}
	/**
	 * create a kleene state of this NFA and assign its name and specification (whether kleene star, kleene plus and so on)
	 *
	 * @param spec the specification of this state (whether kleene star, kleene plus and so on)
	 * @return a pointer to a kleene state
	 */
	typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneStatePtr createKleeneState(string name,
			typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneSpecification spec = KleeneState<
					TinPtr, ToutPtr, TdepPtr>::Star) {
		typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneStatePtr state(
				new KleeneState<TinPtr, ToutPtr, TdepPtr>(stateCountID++, name, spec));
		kleeneStates.push_back(state);
		return state;
	}
	/**
	 * create a final state for this NFA and assign its name
	 * @param name the name of the final state
	 * @return a pointer to a final state
	 */
	typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr createFinalState(string name) {
		typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr final(
				new FinalState<TinPtr, ToutPtr, TdepPtr>(stateCountID++, name));
		finalStates.push_back(final);
		return final;
	}
	/**
	 * create a negated state for this NFA and assign its name
	 * @param name the name of the negated state
	 * @return a pointer to a negated state
	 */
	typename NegationState<TinPtr, ToutPtr, TdepPtr>::NegationStatePtr createNegationState(
			string name) {
		typename NegationState<TinPtr, ToutPtr, TdepPtr>::NegationStatePtr state(
				new NegationState<TinPtr, ToutPtr, TdepPtr>(stateCountID++, name));
		negatedStates.push_back(state);
		return state;
	}
	/**
	 * create a forward edge for this NFA for a given predicate
	 * @param predicate the predicate of this edge
	 * @return a pointer to a forward edge
	 */
	typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr createForwardEdge(
			typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::EdgePredicate predicate) {
		typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr edge(
				new ForwardEdge<TinPtr, ToutPtr, TdepPtr>(edgeCountID++, predicate));
		transitions.push_back(edge);
		return edge;
	}
	/**
	 * create a loop edge for this NFA for a given predicate
	 * @param predicate the predicate of this edge
	 * @return a pointer to a loop edge
	 */
	typename LoopEdge<TinPtr, ToutPtr, TdepPtr>::LoopEdgePtr createLoopEdge(
			typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::EdgePredicate predicate) {
		typename LoopEdge<TinPtr, ToutPtr, TdepPtr>::LoopEdgePtr edge(
				new LoopEdge<TinPtr, ToutPtr, TdepPtr>(edgeCountID++, predicate));
		transitions.push_back(edge);
		return edge;
	}
	/**
	 * create a forward transition for this NFA between two states by an edge
	 * @param src the source state of this transition
	 * @param dest the destination state of this transition
	 * @param edge an edge to connect both the source and destination nodes
	 */
	void createForwardTransition(typename NFAState<TinPtr>::StatePtr src,
			typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr edge,
			typename NFAState<TinPtr>::StatePtr dest) {
		if (edge->getEdgeType() == NFAEdge<TinPtr, ToutPtr, TdepPtr>::Forward) {
			typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr forward =
					std::static_pointer_cast<ForwardEdge<TinPtr, ToutPtr, TdepPtr>>(edge);
			forward->setDestState(dest.get());
			std::static_pointer_cast<NormalState<TinPtr, ToutPtr, TdepPtr>>(src)->addEdge(forward);
		} else
			assert(false);
	}

	/**
	 * create a forward transition for this NFA between two states (given their ids) by an edge (given its ids)
	 * @param src the source state id of this transition
	 * @param dest the destination state id of this transition
	 * @param edge an edge id to connect both the source and destination nodes
	 */
	void createForwardTransition(int src, int edge, int dest) {
		assert(src == dest);
		this->createForwardTransition(getState(src), getEdge(edge),
				getState(dest));
	}
	/**
	 * create a loop transition for this NFA for kleene state (given its id) by a loop edge (given its ids).
	 * this function adds a loop edge predicate to a given state object, this state must be kleene state which has always
	 * @param source the source state id of this transition
	 * @param edge a loop edge id to connect the source with itself
	 */
	void createLoopTransition(int source, int edge) {
		this->createLoopTransition(getState(source), getEdge(edge));
	}
	/**
	 * create a loop transition for this NFA for kleene state  by a loop edge
	 * @param source the source state of this transition
	 * @param edge a loop edge to connect the source with itself
	 */
	void createLoopTransition(typename NFAState<TinPtr>::StatePtr source,
			typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr edge);
	/**
	 * add a loop edge predicate to a given state id, this state must be kleene state which has always
	 * loop edge
	 * @param state the kleene state id to add a loop edge to it
	 * @param predicate the predicate of the kleene state to be assigned to the loop edge
	 */
	void addLoopEdgeToState(int state,
			typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::EdgePredicate predicate);
	/**
	 * get the id of the start state
	 * @return as above
	 */
	int getStartStateID() {
		if (start.get() != NULL)
			return start->getStateID();
		else
			return -1;
	}
	/**
	 * get a pointer to the start state
	 * @return as above
	 */
	StartState<TinPtr, ToutPtr, TdepPtr>* getStartState() const {
		return start.get();
	}
	/**
	 * get a vector of final states in the NFA
	 * @return as above
	 */
	const std::vector<typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr>& getFinalStates() const {
		return this->finalStates;
	}

	/**
	 * set the final states for this NFA
	 * @param a vector of final states
	 */
	void setFinalStates(
			std::vector<typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr> finalStates) {
		for (int i = 0; i < finalStates.size(); i++) {
			addFinalState(finalStates[i]);
		}
	}
	/**
	 * add a final state to this NFA
	 * @param a final state to be added to this NFA
	 */
	void addFinalState(typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr final);

	/**
	 * set the kleene states states of this NFA
	 * @param KleeneStates the kleene states to set
	 */
	void setKleeneStates(
			std::vector<typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneStatePtr> kleeneStates) {
		for (int i = 0; i < kleeneStates.size(); i++) {
			addKleeneState(kleeneStates[i]);
		}
	}
	/**
	 * get a vector of kleene states in the NFA
	 * @return as above
	 */
	const std::vector<typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneStatePtr>& getKleeneStates() const {
		return this->kleeneStates;
	}
	/**
	 * add a kleene state to this NFA
	 * @param a kleene state to be added to this NFA
	 */
	void addKleeneState(typename KleeneState<TinPtr, ToutPtr, TdepPtr>::KleeneStatePtr kleeneState) {
		if (std::find(kleeneStates.begin(), kleeneStates.end(), kleeneState)
				== kleeneStates.end()) {
			kleeneStates.push_back(kleeneState);
		}
	}

	/**
	 * set the negated states states of this NFA
	 * @param negated_states the negated states to set
	 */
	void setNegatedStates(
			std::vector<typename NegationState<TinPtr, ToutPtr, TdepPtr>::NegationStatePtr> negatedStates) {
		for (int i = 0; i < negatedStates.size(); i++) {
			addNegatedState(negatedStates[i]);
		}
	}
	/**
	 * get a vector of negated states in the NFA
	 * @return as above
	 */
	const std::vector<typename NegationState<TinPtr, ToutPtr, TdepPtr>::NegationStatePtr>& getNegatedStates() const {
		return negatedStates;
	}
	/**
	 * add a negated state to this NFA
	 * @param a negated state to be added to this NFA
	 */
	void addNegatedState(
			typename NegationState<TinPtr, ToutPtr, TdepPtr>::NegationStatePtr negatedState) {
		if (std::find(negatedStates.begin(), negatedStates.end(), negatedState)
				== negatedStates.end()) {
			negatedStates.push_back(negatedState);
		}
	}

	/**
	 * set the normal states states of this NFA
	 * @param NormalStates the normal states to set
	 */
	void setNormalStates(
			std::vector<typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr> normalStates) {
		for (int i = 0; i < normalStates.size(); i++) {
			addNormalState(normalStates[i]);
		}
	}
	/**
	 * get a vector of normal states in the NFA
	 * @return as above
	 */
	const std::vector<typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr>& getNormalStates() const {
		return normalStates;
	}
	/**
	 * add a normal state to this NFA
	 * @param negated_state a negated state to be added to this NFA
	 */
	void addNormalState(typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr NormalState);

	/**
	 * get a vector of intermediate states including negated, kleene and normal states in the NFA
	 * @return as above
	 */
	std::vector<typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr> getInterStates() const;

	/**
	 * add forward edges for a particular state, this state can be start, normal, negated and kleene states
	 * each states in our system has multiple forward edges except the final states
	 * @param state a state to assign forward edges to it
	 * @param edges a vector of forward edges to be assigned to a state
	 */

	void addForwardEdges(typename NFAState<TinPtr>::StatePtr state,
			std::vector<typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr> edges);
	/**
	 * add a forward edge for a particular state, this state can be start, normal, negated and kleene states
	 * each states in our system has multiple forward edges except the final states
	 * @param state a state to assign this forward edge to it
	 * @param edge an edge to be assigned to a state
	 */
	void addForwardEdge(typename NFAState<TinPtr>::StatePtr state,
			typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr edge);
	/**
	 * add forward edges by their ids to a particular state given its id, this state can be start, normal, negated and kleene states
	 * each states in our system has multiple forward edges except the final states
	 * @param state a state id to assign forward edges to it
	 * @param edges a vector of forward edge ids to be assigned to a state
	 */
	void addForwardEdges(int state, std::vector<int> edges);
	/**
	 * add a forward edge by its id for a particular state given its id, this state can be start, normal, negated and kleene states
	 * each states in our system has multiple forward edges except the final states
	 * @param state a state id to assign this forward edge to it
	 * @param edge an edge id to be assigned to a state
	 */
	void addForwardEdge(int state, int edge);

	/**
	 * get a vector of the transitions in the NFA
	 * @return as above
	 */
	const std::vector<typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr>& getTransitions() const {
		return transitions;
	}
	/*
	 void addDependencyEdges(typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr edgeSource, typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr edgeDepend,
	 int attrIdx, StateDependency::OperationDependency op, StateDependency::OperationType type = StateDependency::Double );
	 vector<StateDependencyPtr> getSourceOf() const;
	 void setSourceOf(vector<StateDependencyPtr> sourceOf);*/
	/**
	 * return the number of states which has kleene type
	 * @return as above
	 */
	int getKleeneStatesCount() const {
		return kleeneStates.size();
	}
	/**
	 * return the number of negated states
	 * @return as above
	 */
	int getNegationStatesCount() const {
		return negatedStates.size();
	}

	void setDependency(initDependency init, updateDependency update) {
		this->init = init ;
		this->update = update;

	}
	void print (ostream& out = std::cout) ;

	initDependency init ;

	updateDependency update ;
};
} /* namespace pfabric */

namespace pfabric {

template <class TinPtr, class ToutPtr, class TdepPtr>
std::vector<typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr> NFAController<TinPtr, ToutPtr, TdepPtr>::getInterStates() const {
	std::vector<typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr> result;
	copy(normalStates.begin(), normalStates.end(), back_inserter(result));
	copy(kleeneStates.begin(), kleeneStates.end(), back_inserter(result));
	copy(negatedStates.begin(), negatedStates.end(), back_inserter(result));
	return result;
}

template <class TinPtr, class ToutPtr, class TdepPtr>
void NFAController<TinPtr, ToutPtr, TdepPtr>::addFinalState(typename FinalState<TinPtr, ToutPtr, TdepPtr>::FinalStatePtr final) {
	if (std::find(finalStates.begin(), finalStates.end(), final)
			== finalStates.end()) {
		finalStates.push_back(final);
	}
}

/*
void NFAController::addDependencyEdges(NFAEdgePtr edgeSource,
		NFAEdgePtr edgeDepend, int attrIdx,
		StateDependency::OperationDependency op,
		StateDependency::OperationType type) {

	StateDependencyPtr dependency(
			new StateDependency(edgeSource, edgeDepend, attrIdx, op, type));
	edgeSource->addStateSource(dependency);
	edgeDepend->addStateDependeny(dependency);
}*/




template <class TinPtr, class ToutPtr, class TdepPtr>
void NFAController<TinPtr, ToutPtr, TdepPtr>::addForwardEdges(typename NFAState<TinPtr>::StatePtr state,
		std::vector<typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr> edges) {
	if (state->getStateType() == NFAState<TinPtr>::Normal)
		boost::static_pointer_cast<NormalState<TinPtr, ToutPtr, TdepPtr>>(state)->setForwardEdges(
				edges);
	else
		assert(false);
}

template <class TinPtr, class ToutPtr, class TdepPtr>
void NFAController<TinPtr, ToutPtr, TdepPtr>::addForwardEdge(typename NFAState<TinPtr>::StatePtr state, typename ForwardEdge<TinPtr, ToutPtr, TdepPtr>::ForwardEdgePtr edge) {
	if (state->getStateType() == NFAState<TinPtr>::Normal)
		boost::static_pointer_cast<NormalState<TinPtr, ToutPtr, TdepPtr>>(state)->addEdge(edge);
	else
		assert(false);
}
/*
 void NFAController::add_ForwardEdges(int state, std::vector<int> edges) {
 std::vector<ForwardNFAEdgePtr> f_edges;
 for (int i = 0; i < edges.size(); i++) {
 ForwardNFAEdgePtr e;
 if ((e = getEdge(edges[i])).IsValid())
 f_edges.push_back(e);
 }
 add_ForwardEdges(getState(state), f_edges);
 }
 */
template <class TinPtr, class ToutPtr, class TdepPtr>
void NFAController<TinPtr, ToutPtr, TdepPtr>::addForwardEdge(int state, int edge) {
	addForwardEdge(getState(state),
			boost::static_pointer_cast<ForwardEdge<TinPtr, ToutPtr, TdepPtr>>(getEdge(edge)));
}

template <class TinPtr, class ToutPtr, class TdepPtr>
typename NFAState<TinPtr>::StatePtr NFAController<TinPtr, ToutPtr, TdepPtr>::getState(int id) {
	if (start->getStateID() == id)
		return start;
	for (int i = 0; i < negatedStates.size(); i++) {
		if (negatedStates[i]->getStateID() == id) {
			return negatedStates[i];
		}

	}
	for (int i = 0; i < normalStates.size(); i++) {
		if (normalStates[i]->getStateID() == id) {
			return normalStates[i];
		}

	}
	for (int i = 0; i < finalStates.size(); i++) {
		if (finalStates[i]->getStateID() == id) {
			return finalStates[i];
		}
	}
	return NFAState<TinPtr>::StatePtr();
}

template <class TinPtr, class ToutPtr, class TdepPtr>
typename NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr NFAController<TinPtr, ToutPtr, TdepPtr>::getEdge(int id) {
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i]->getID() == id) {
			return transitions[i];
		}
	}
	return NFAEdge<TinPtr, ToutPtr, TdepPtr>::NFAEdgePtr();
}



template <class TinPtr, class ToutPtr, class TdepPtr>
void NFAController<TinPtr, ToutPtr, TdepPtr>::addNormalState(typename NormalState<TinPtr, ToutPtr, TdepPtr>::NormalStatePtr normalState) {
	if (std::find(normalStates.begin(), normalStates.end(), normalState)
			== normalStates.end()) {
		normalStates.push_back(normalState);
	}
}

template <class TinPtr, class ToutPtr, class TdepPtr>
void NFAController<TinPtr, ToutPtr, TdepPtr>::print (ostream& out) {
	out << this->start->getStateName() << std::endl;
}


}
#endif /* NFAController_hpp_ */
