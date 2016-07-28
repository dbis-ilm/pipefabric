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

#ifndef NFAStructure_hpp_
#define NFAStructure_hpp_
#include <vector>
#include "NFAController.hpp"
#include "Instance.hpp"
#include "relatedvalue/RelatedStateValuePrevious.hpp"
#include "state/StartState.hpp"
#include "edge/NFAEdge.hpp"
#include "util/Partition.hpp"

namespace pfabric {
template<class TinPtr, class ToutPtr, class TdepPtr>
struct compare {
	compare(KleeneState<TinPtr, ToutPtr, TdepPtr>* val) :
			val_(val) {
	}
	bool operator()(const std::pair<KleeneState<TinPtr, ToutPtr, TdepPtr>*, short>& elem) const {
		return val_ == elem.first;
	}
private:
	KleeneState<TinPtr, ToutPtr, TdepPtr>* val_;
};
//template<class Tin, class Tout>
//class NFAStructure;
template<class TinPtr, class ToutPtr, class TdepPtr>
class StructurePool;

template<class TinPtr, class ToutPtr, class TdepPtr>
class NFAStructure {
	mutable boost::atomic<short> refCount;

private:
	/**
	 * store the events of this structure which contribute on complex event
	 */
	std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr> events;

	/**
	 * which state the structure is at
	 */
	NFAState<TinPtr>* currentState;

	/**
	 * boolean indicating whether the structure is complete to make a match
	 */
	bool complete;
	/**
	 * our working NFA
	 */
	typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr  nfa;

	/**
	 * store related values
	 */
	TdepPtr relatedValues;

	/*
	 * the partition of  this structure
	 */
	Partition<TinPtr>* equality;

	std::vector<std::pair<KleeneState<TinPtr, ToutPtr, TdepPtr>*, short> > kleeneState;



public:
	typedef boost::intrusive_ptr<NFAStructure<TinPtr, ToutPtr, TdepPtr>> NFAStructurePtr;
	/**
	 * constructor
	 * @param start the current state is set to start state
	 */
	NFAStructure(typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr  nfa);

	/**
	 * virtual destructor
	 */
	virtual ~NFAStructure() { delete equality; }

	/**
	 * Get the current state
	 * @return the current state
	 */
	NFAState<TinPtr>* getCurrentState() const { return this->currentState; }
	/**
	 * set the current state
	 * @param current_state the current state to set
	 */
	void setCurrentState(NFAState<TinPtr>* cur) {
		assert(currentState);
		this->currentState = cur;
	}
	/**
	 * get all event taht match this structure
	 * @return all event for this structure
	 */
	const std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr>& getEvents() const { return this->events; }
	/**
	 * set all event ids for this structure
	 * @param events all event ids for this structure
	 */
	void setEvents(std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr> events) {	this->events = events; }
	/**
	 * Checks whether this structure is ready to make a matcht
	 * @return the complete flag
	 */

	bool isComplete() const { return this->complete; }
	/**
	 * set the complete flag
	 * @param complete the complete flag to set
	 */
	void setComplete(bool complete) { this->complete = complete; }

	/**
	 * get the nfa controller
	 * @return the nfa
	 */
	typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr getNFA() const { return this->nfa; }
	/**
	 * set the nfa
	 * @param nfa the nfa to set
	 */
	void setNFA(typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr  nfa);
	/**
	 * Adds an event to this structure, and makes necessary updates
	 * @param event the event to be added
	 * @param the current edge
	 */
	void addEvent(const TinPtr& event, NFAEdge<TinPtr, ToutPtr, TdepPtr>* currentEdge);
	/**
	 * Clones the structure
	 * @return new structure
	 */
	//typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr clone( StructurePool<TinPtr, ToutPtr, TdepPtr>* pool) ;
	/**
	 * get a particular event
	 */
	typename Instance<TinPtr, ToutPtr>::InstancePtr getEvent(long index) const {
		assert(index < events.size());
		return this->events[index];
	}
	/**
	 * get a time stamp for particulat event
	 */
	Timestamp getEventTimestamp(long index) const {
		assert(index < events.size());
		return this->events[index]->getInstanceTimestamp();
	}
	/**
	 * get the timestamp for the last event for this structure
	 */
	Timestamp getLastEventTimestamp() const { return this->events.back()->getInstanceTimestamp(); }
	/**
	 * get the timestamp for the first event for this structure
	 */
	Timestamp getFirstEventTimestamp() const { return this->events.front()->getInstanceTimestamp(); }

	/**
	 * print some information about this structure
	 * @param os
	 */
	void print(std::ostream & os);
	/**
	 * Equality comparison operator.
	 * @param rhs The input structure for this structure to compare to
	 * @return True iff this structure has the same information as that of rhs
	 */
	bool operator==(const NFAStructure & rhs) { return this == &rhs; }

	/**
	 * Inequality comparison operator.
	 * @param rhs The input structure for this structure to compare to
	 * @return True iff this structure does not the same information as that of rhs
	 */
	bool operator!=(const NFAStructure & rhs) { return this == &rhs; }
	/**
	 * get the sequence or number of events in this structure
	 * @return as above
	 */
	int getSequence() const { return this->events.size(); }

	friend void intrusive_ptr_add_ref(const NFAStructure *x) {
		x->refCount.fetch_add(1, boost::memory_order_relaxed);
	}
	friend void intrusive_ptr_release(const NFAStructure *x) {
		if (x->refCount.fetch_sub(1, boost::memory_order_release) == 1) {
			boost::atomic_thread_fence(boost::memory_order_acquire);
			delete x;
		}
	}

	const TdepPtr& getRelatedValue() {return relatedValues;}

	void setEqualityValue(Partition<TinPtr>* par) { this->equality = par; }

	Partition<TinPtr>* getEqualityValue() { return equality; }

	short getCurrentKleene(KleeneState<TinPtr, ToutPtr, TdepPtr>* kState) const {
		return std::find_if(kleeneState.begin(), kleeneState.end(),
			compare<TinPtr, ToutPtr, TdepPtr>((KleeneState<TinPtr, ToutPtr, TdepPtr>*) (currentState)))->second;
	};

};

}

namespace pfabric {

template<class TinPtr, class ToutPtr, class TdepPtr>
NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructure(typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr  nfa) :
		refCount(0), kleeneState(nfa->getKleeneStatesCount()) {

	this->nfa = nfa;
	this->currentState = nfa->getStartState();
	this->complete = false;
	for (int i = 0; i < nfa->getKleeneStatesCount(); i++) {
		kleeneState[i] = std::make_pair(nfa->getKleeneStates()[i].get(), 0);
	}
	relatedValues = this->nfa->init();
}

template<class TinPtr, class ToutPtr, class TdepPtr>
void NFAStructure<TinPtr, ToutPtr, TdepPtr>::addEvent(const TinPtr& event,
		NFAEdge<TinPtr, ToutPtr, TdepPtr>* currentEdge) {
	typename Instance<TinPtr, ToutPtr>::InstancePtr inst(new Instance<TinPtr, ToutPtr>(event));

	this->events.push_back(inst);

	inst->setSequenceInComplex(this->events.size());
	inst->setState(this->currentState->getStateName());

	this->nfa->update( relatedValues, currentEdge->getID(), event);

	if (currentEdge->getEdgeType() == NFAEdge<TinPtr, ToutPtr, TdepPtr>::Forward) {
		this->currentState =
				((ForwardEdge<TinPtr, ToutPtr, TdepPtr>*) (currentEdge))->getDestState();
	} else if (currentEdge->getEdgeType() == NFAEdge<TinPtr, ToutPtr, TdepPtr>::Loop) {

		typename vector<std::pair<KleeneState<TinPtr, ToutPtr, TdepPtr>*, short> >::iterator res = std::find_if(
				kleeneState.begin(), kleeneState.end(),
				compare<TinPtr, ToutPtr, TdepPtr>((KleeneState<TinPtr, ToutPtr, TdepPtr>*) (currentState)));
		res->second = res->second + 1;
	}
	if (currentState->getStateType() == NFAState<TinPtr>::Final) {
		this->complete = true;

	} else if (currentState->getStateType() == NFAState<TinPtr>::Kleene) {
		//initRelatedValue(event,
			//	((KleeneState*) this->currentState)->getLoopEdge());
	}
}
/*
template<class Tin, class Tout>
typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr NFAStructure<TinPtr, ToutPtr, TdepPtr>::clone(StructurePool<TinPtr, ToutPtr, TdepPtr>* pool) {
	int counter = 0;
	std::vector<typename Instance<TinPtr, ToutPtr, TdepPtr>::InstancePtr> vector(this->events.size());
	for (std::vector<typename Instance<TinPtr, ToutPtr, TdepPtr>::InstancePtr>::iterator it = this->events.begin();
			it != this->events.end(); it++) {
		vector.at(counter++) = *it;
	}
	counter = 0;
	//vector_related_value related(this->related_values.size());

	for (std::vector<typename Instance<TinPtr, ToutPtr, TdepPtr>::InstancePtr>::iterator it = this->events.begin();
			it != this->events.end(); it++) {
		vector.at(counter++) = *it;
	}

	NFAStructurePtr cloneStr = this->pool->getStructure(this->nfa, equality->clone());

	cloneStr->setComplete(this->complete);
	cloneStr->setEvents(vector);
	cloneStr->setCurrentState(this->currentState);*/
	//return cloneStr;

//}
/*
template<class Tin, class Tout>
void NFAStructure<TinPtr, ToutPtr, TdepPtr>::addStateSource() {
	for (int i = 0; i < this->nfa->getTransitions().size(); i++) {
		relatedValues.addRelatedValue(
				this->nfa->getTransitions()[i]->getSourceOf());
	}
}
template<class Tin, class Tout>
any NFAStructure<TinPtr, ToutPtr, TdepPtr>::getRelatedValue(int edge_id, int index) {

	std::vector<RelatedStateValuePtr> values = relatedValues.at(edge_id);
	return values[index]->getValue();
}
template<class Tin, class Tout>
void NFAStructure<TinPtr, ToutPtr, TdepPtr>::updateRelatedValue(const tuple_ptr& inst,
		NFAEdge* edge) {

	if (edge->hasRelatedValues()) {
		const vector<RelatedStateValuePtr>& values = relatedValues.at(
				edge->getID());
		for (int i = 0; i < values.size(); i++) {
			values[i]->updateValue(inst);
		}
	}
}
template<class Tin, class Tout>
void NFAStructure<TinPtr, ToutPtr, TdepPtr>::initRelatedValue(const TinPtr& inst,
		NFAEdge* edge) {

	if (edge->hasRelatedValues()) {
		const vector<RelatedStateValuePtr>& values = relatedValues.at(
				edge->getID());
		for (int i = 0; i < values.size(); i++) {
			values[i]->initValue(inst);
		}
	}
}*/

}
#endif /* NFAStructure_hpp_ */
