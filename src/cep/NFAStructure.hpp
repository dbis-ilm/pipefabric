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
template<class Tin, class Tout, class Tdep>
struct compare {
	compare(KleeneState<Tin, Tout, Tdep>* val) :
			val_(val) {
	}
	bool operator()(const std::pair<KleeneState<Tin, Tout, Tdep>*, short>& elem) const {
		return val_ == elem.first;
	}
private:
	KleeneState<Tin, Tout, Tdep>* val_;
};
//template<class Tin, class Tout>
//class NFAStructure;
template<class Tin, class Tout, class Tdep>
class StructurePool;

template<class Tin, class Tout, class Tdep>
class NFAStructure {
	typedef boost::intrusive_ptr<Tin> TinPtr;
	typedef boost::intrusive_ptr<Tin> ToutPtr;
	mutable boost::atomic<short> refCount;

private:
	/**
	 * store the events of this structure which contribute on complex event
	 */
	std::vector<typename Instance<Tin, Tout>::InstancePtr> events;

	/**
	 * which state the structure is at
	 */
	NFAState<Tin>* currentState;

	/**
	 * boolean indicating whether the structure is complete to make a match
	 */
	bool complete;
	/**
	 * our working NFA
	 */
	typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr  nfa;

	/**
	 * store related values
	 */
	Tdep relatedValues;

	/*
	 * the partition of  this structure
	 */
	Partition<TinPtr>* equality;

	std::vector<std::pair<KleeneState<Tin, Tout, Tdep>*, short> > kleeneState;



public:
	typedef boost::intrusive_ptr<NFAStructure<Tin, Tout, Tdep>> NFAStructurePtr;
	/**
	 * constructor
	 * @param start the current state is set to start state
	 */
	NFAStructure(typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr  nfa);

	/**
	 * virtual destructor
	 */
	virtual ~NFAStructure() { delete equality; }

	/**
	 * Get the current state
	 * @return the current state
	 */
	NFAState<Tin>* getCurrentState() const { return this->currentState; }
	/**
	 * set the current state
	 * @param current_state the current state to set
	 */
	void setCurrentState(NFAState<Tin>* cur) {
		assert(currentState);
		this->currentState = cur;
	}
	/**
	 * get all event taht match this structure
	 * @return all event for this structure
	 */
	const std::vector<typename Instance<Tin, Tout>::InstancePtr>& getEvents() const { return this->events; }
	/**
	 * set all event ids for this structure
	 * @param events all event ids for this structure
	 */
	void setEvents(std::vector<typename Instance<Tin, Tout>::InstancePtr> events) {	this->events = events; }
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
	typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr getNFA() const { return this->nfa; }
	/**
	 * set the nfa
	 * @param nfa the nfa to set
	 */
	void setNFA(typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr  nfa);
	/**
	 * Adds an event to this structure, and makes necessary updates
	 * @param event the event to be added
	 * @param the current edge
	 */
	void addEvent(const TinPtr& event, NFAEdge<Tin, Tout, Tdep>* currentEdge);
	/**
	 * Clones the structure
	 * @return new structure
	 */
	//typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr clone( StructurePool<Tin, Tout, Tdep>* pool) ;
	/**
	 * get a particular event
	 */
	typename Instance<Tin, Tout>::InstancePtr getEvent(long index) const {
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

	const Tdep& getRelatedValue() {return relatedValues;}

	void setEqualityValue(Partition<TinPtr>* par) { this->equality = par; }

	Partition<TinPtr>* getEqualityValue() { return equality; }

	short getCurrentKleene(KleeneState<Tin, Tout, Tdep>* kState) const {
		return std::find_if(kleeneState.begin(), kleeneState.end(),
			compare<Tin, Tout, Tdep>((KleeneState<Tin, Tout, Tdep>*) (currentState)))->second;
	};

};

}

namespace pfabric {

template<class Tin, class Tout, class Tdep>
NFAStructure<Tin, Tout, Tdep>::NFAStructure(typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr  nfa) :
		refCount(0), kleeneState(nfa->getKleeneStatesCount()) {

	this->nfa = nfa;
	this->currentState = nfa->getStartState();
	this->complete = false;
	for (int i = 0; i < nfa->getKleeneStatesCount(); i++) {
		kleeneState[i] = std::make_pair(nfa->getKleeneStates()[i].get(), 0);
	}
	relatedValues = this->nfa->init();
}

template<class Tin, class Tout, class Tdep>
void NFAStructure<Tin, Tout, Tdep>::addEvent(const TinPtr& event,
		NFAEdge<Tin, Tout, Tdep>* currentEdge) {
	typename Instance<Tin, Tout>::InstancePtr inst(new Instance<Tin, Tout>(event));

	this->events.push_back(inst);

	inst->setSequenceInComplex(this->events.size());
	inst->setState(this->currentState->getStateName());

	this->nfa->update( relatedValues, currentEdge->getID(), event);

	if (currentEdge->getEdgeType() == NFAEdge<Tin, Tout, Tdep>::Forward) {
		this->currentState =
				((ForwardEdge<Tin, Tout, Tdep>*) (currentEdge))->getDestState();
	} else if (currentEdge->getEdgeType() == NFAEdge<Tin, Tout, Tdep>::Loop) {

		typename vector<std::pair<KleeneState<Tin, Tout, Tdep>*, short> >::iterator res = std::find_if(
				kleeneState.begin(), kleeneState.end(),
				compare<Tin, Tout, Tdep>((KleeneState<Tin, Tout, Tdep>*) (currentState)));
		res->second = res->second + 1;
	}
	if (currentState->getStateType() == NFAState<Tin>::Final) {
		this->complete = true;

	} else if (currentState->getStateType() == NFAState<Tin>::Kleene) {
		//initRelatedValue(event,
			//	((KleeneState*) this->currentState)->getLoopEdge());
	}
}
/*
template<class Tin, class Tout>
typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr NFAStructure<Tin, Tout, Tdep>::clone(StructurePool<Tin, Tout, Tdep>* pool) {
	int counter = 0;
	std::vector<typename Instance<Tin, Tout, Tdep>::InstancePtr> vector(this->events.size());
	for (std::vector<typename Instance<Tin, Tout, Tdep>::InstancePtr>::iterator it = this->events.begin();
			it != this->events.end(); it++) {
		vector.at(counter++) = *it;
	}
	counter = 0;
	//vector_related_value related(this->related_values.size());

	for (std::vector<typename Instance<Tin, Tout, Tdep>::InstancePtr>::iterator it = this->events.begin();
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
void NFAStructure<Tin, Tout, Tdep>::addStateSource() {
	for (int i = 0; i < this->nfa->getTransitions().size(); i++) {
		relatedValues.addRelatedValue(
				this->nfa->getTransitions()[i]->getSourceOf());
	}
}
template<class Tin, class Tout>
any NFAStructure<Tin, Tout, Tdep>::getRelatedValue(int edge_id, int index) {

	std::vector<RelatedStateValuePtr> values = relatedValues.at(edge_id);
	return values[index]->getValue();
}
template<class Tin, class Tout>
void NFAStructure<Tin, Tout, Tdep>::updateRelatedValue(const tuple_ptr& inst,
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
void NFAStructure<Tin, Tout, Tdep>::initRelatedValue(const TinPtr& inst,
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
