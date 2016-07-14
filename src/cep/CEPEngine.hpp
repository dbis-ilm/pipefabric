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

#ifndef CEPEngine_hpp_
#define CEPEngine_hpp_
//#include "EventBuffer.hpp"
#include "Instance.hpp"
//#include "Matcher.hpp"
#include <list>
#include <map>
#include "StructurePool.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "NFAController.hpp"
#include "util/ValueIDMultimap.hpp"
/**
 * The main engine to process CEP
 */
namespace pfabric {
template<class Tin, class Tout, class Tdep>
class Matcher;
//class GCStructures;
template<class Tin, class Tout, class Tdep>
class CEPEngine {
	typedef boost::intrusive_ptr<Tin> TinPtr;
protected:
	/**
	 * CEP manager to notify the system with the matched events
	 */
	Matcher<Tin, Tout, Tdep> *manager;
	/**
	 * statistical counter to store  number of matches
	 */
	long counter;
	/**
	 * a pool of structures
	 */
	StructurePool<Tin, Tout, Tdep>* pool;
	/**
	 * to be deleted structures, these can be outputted structures or violated time constraint structures
	 */
	std::list<typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr> deletedStructures;
	/**
	 * the global NFA
	 */
	typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr  nfa;
	/**
	 * create new structure, the event in the parameter will be the first matched event
	 * @param event an event to start the structure (sequence) with it
	 */
	void createStartStructure(const TinPtr& event);

	/**
	 * the partition structure which the structures can be stored in the pool accordingly
	 */
	Partition<TinPtr>* equalityPar;
	/**
	 * sequence collector to remove unnecessary sequences
	 */
	//GCStructures* gc;

	/**
	 * an indicator to notify the sequence collector to remove garbage sequences
	 */

	boost::atomic<bool> cgIndicator;

	/**
	 * check the time window constraint (within) for the current event and a structure
	 * @param event
	 * @param str
	 * @return
	 */
	bool checkWindowTime(const TinPtr& event, const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str);
	/**
	 * check edge predicates
	 * @param event
	 * @param str
	 * @return
	 */
	int checkPredicate(const TinPtr& event, const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str,
			typename NFAState<Tin>::StateType &type);
public:
	/**
	 * constructor to receive the CEP manager
	 * @param manager
	 */
	CEPEngine(Matcher<Tin, Tout, Tdep> *manager): counter(0), cgIndicator(false) {
		this->manager = manager;
		this->nfa = typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr (new NFAController<Tin, Tout, Tdep>());
		this->pool = new StructurePool<Tin, Tout, Tdep>();
		this->equalityPar = new SequencePartition<TinPtr>;
		windowConst = new WindowStruct;
		windowConst->window = WindowStruct::NoConstraint;
	}
	/**
	 * destructor
	 */
	virtual ~CEPEngine();
	/**
	 * print number of matches
	 * @param os the output stream object
	 */
	virtual void printNumMatches(std::ostream& os) {
		os << this->pool->getNumInsertions() << " " << this->pool->size();
	}

public:

	struct WindowStruct {
		enum WindowContstant {
			FirstLastEvents, FromLastEvents, FromToEvents, NoConstraint
		};
		WindowContstant window;
		int eventFrom;
		int eventTo;
		long period;
	};
	/**
	 * virtual function to implement different processing approaches such as next matches, all matches ....
	 * @param event the current event
	 * @param str a structure to investigate the event with respect to it
	 */
	virtual void runEngine(const TinPtr& event)=0;
	/**
	 * get the structures pool
	 * @return structures pool
	 */
	StructurePool<Tin, Tout, Tdep>* getStructurePool() const {
		return this->pool;
	}
	/**
	 * set the structures pool
	 * @param pool the structures pool
	 */
	void setStructurePool(StructurePool<Tin, Tout, Tdep>* pool) {
		this->pool = pool ;
	}
	/**
	 * get the structures to be deleted
	 * @return the structures to be deleted
	 */
	std::list<typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr> getDeletedStructures() const {
		return this->deletedStructures;
	}
	/**
	 *  set the structures to be deleted
	 * @param deletedStructures the structures to be deleted
	 */
	void setDeletedStructures(std::list<typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr> deletedStructures) {
		this->deletedStructures = deletedStructures;
	}
	/**
	 * get our working NFA
	 * @return  our working NFA
	 */
	const typename NFAController<Tin, Tout, Tdep>::NFAControllerPtr  getNFA() const {
		return this->nfa;
	}
	/**
	 * set our working NFA
	 * @param nfa our working NFA
	 */
	void setNFA(NFAController<Tin, Tout, Tdep>* nfa) { this->nfa = nfa; }
	/**
	 * clean unwanted structures from current run
	 */
	void runGCstructures();
	/**
	 * return the number of matches
	 */
	long getNumMatches() const {
		return this->counter;
	}
	/**
	 * set the partition object itself
	 */
	void setEquality(Partition<TinPtr>* equality) {
		if (equalityPar)
			delete equalityPar;
		equalityPar = equality;
	}
	/**
	 * set the window constraint parameters by the cep engine
	 */
	void setWindowConstraint(long period, int fromEvent = -1, int toEvent = -1);
	/**
	 * get the window from the cep engine
	 */
	WindowStruct* getWindow() const { return this->windowConst; }

	/**
	 * return if the CEP has window constraint
	 */
	bool hasWindow() { return !(windowConst->window == WindowStruct::NoConstraint); }
protected:

	/**
	 * specify the window (within) constraint
	 */
	WindowStruct* windowConst;
};
}

namespace pfabric {


template<class Tin, class Tout, class Tdep>
CEPEngine<Tin, Tout, Tdep>::~CEPEngine() {
	delete this->pool;
	delete this->equalityPar;
	delete this->windowConst;
}

template<class Tin, class Tout, class Tdep>
bool CEPEngine<Tin, Tout, Tdep>::checkWindowTime(const TinPtr& event,
		const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str) {
	if (windowConst->window == WindowStruct::FirstLastEvents) {
		if (event->getTimestamp() - str->getFirstEventTimestamp()
				<= windowConst->period) {
			return true;
		}
	} else if (windowConst->window == WindowStruct::FromLastEvents) {
		if (event->getTimestamp()
				- str->getEventTimestamp(windowConst->eventFrom)
				<= windowConst->period)
			return true;
	} else if (windowConst->window == WindowStruct::FromToEvents) {
		if (str->getEventTimestamp(windowConst->eventTo)
				- str->getEventTimestamp(windowConst->eventFrom)
				<= windowConst->period)
			return true;
	}
	return false;
}
template<class Tin, class Tout, class Tdep>
int CEPEngine<Tin, Tout, Tdep>::checkPredicate(const TinPtr& event,
		const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str, typename NFAState<Tin>::StateType &type) {

	if (str->getCurrentState()->getStateType() == NFAState<Tin>::Normal) {
		//creat_new_structure(event, current_state);
		auto current = (NormalState<Tin, Tout, Tdep>*)str->getCurrentState();
		for (int i = 0; i < current->getNumEdges(); i++) {
			if (current->getForwardEdgeByIndex(i)->evaluate(event, str)) {
				return i;
			}
		}
	} else if (str->getCurrentState()->getStateType()
			== NFAState<Tin>::Kleene) {

		//try go to the next
		auto current = (KleeneState<Tin, Tout, Tdep>*)(
				str->getCurrentState());
		bool forwardOK = false;
		switch (current->getSpecification()) {
		case KleeneState<Tin, Tout, Tdep>::Star:
		case KleeneState<Tin, Tout, Tdep>::Question: {
			forwardOK = true;
			break;
		}
		case KleeneState<Tin, Tout, Tdep>::Plus: {
			if (str->getCurrentKleene(current) == 1) {
				forwardOK = true;
			}
			break;
		}
		case KleeneState<Tin, Tout, Tdep>::Restricted:
			if (str->getCurrentKleene(current)
					>= current->getLoopEdge()->getNumOfLoop()) {
				forwardOK = true;
			}
			break;
		}
		if (forwardOK) {
			for (int i = 0; i < current->getNumEdges(); i++) {
				if (current->getForwardEdgeByIndex(i)->evaluate(event,
						str)) {
					return i;
				}
			}
		}
		auto loop = current->getLoopEdge();
		if (loop->evaluate(event, str)) {
			switch (current->getSpecification()) {
			case KleeneState<Tin, Tout, Tdep>::Star:
			case KleeneState<Tin, Tout, Tdep>::Question: {
				str->addEvent(event, loop);
				break;
			}
			case KleeneState<Tin, Tout, Tdep>::Plus: {
				if (str->getCurrentKleene(current) == 0) {
					str->addEvent(event, loop);
				}
				break;
			}
			case KleeneState<Tin, Tout, Tdep>::Restricted: {
				if (str->getCurrentKleene(current)
						< current->getLoopEdge()->getNumOfLoop()) {
					str->addEvent(event, loop);
				}
				break;
			}
			}
		}
	} else if (str->getCurrentState()->getStateType()
			== NFAState<Tin>::Negation) {
		auto current = (NormalState<Tin, Tout, Tdep>*)str->getCurrentState();
		do {
			for (int i = 0; i < current->getNumEdges(); i++) {
				if (current->getForwardEdgeByIndex(i)->evaluate(event,
						str)) {
					type = NFAState<Tin>::Negation;
					return -1;
				}
			}
			current = (NormalState<Tin, Tout, Tdep>*)(
					current->getForwardEdgeByIndex(0)->getDestState());
		} while (current->getStateType() == NFAState<Tin>::Negation);
		if (current->getStateType() == NFAState<Tin>::Final)
			return 0;
		for (int i = 0; i < current->getNumEdges(); i++) {
			if (current->getForwardEdgeByIndex(i)->evaluate(event, str)) {
				str->setCurrentState(current);
				return i;
			}
		}
	}
	return -1;
}

template<class Tin, class Tout, class Tdep>
void CEPEngine<Tin, Tout, Tdep>:: runGCstructures() {
	for (int i = 0; i < this->deletedStructures.size(); i++) {
		const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str = this->deletedStructures.front();
		auto par = str->getEqualityValue();
		typename ValueIDMultimap<typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr, TinPtr>::MultimapPair iterPair =
				this->pool->getValue(par);

		typename ValueIDMultimap<typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr, TinPtr>::MultimapConstIterator it = iterPair.first;
		for (; it != iterPair.second; ++it) {
			if (it->second == str) {
				pool->removeValue(it);
				break;
			}
		}
		this->deletedStructures.pop_front();
	}
}
template<class Tin, class Tout, class Tdep>
void CEPEngine<Tin, Tout, Tdep>::createStartStructure(const TinPtr& event) {
	auto start = this->nfa->getStartState();
	for (int i = 0; i < start->getNumEdges(); i++) {
		if (start->getForwardEdgeByIndex(i)->evaluate(event, NULL)) {
			equalityPar->generateValues(event);
			auto newStructure = this->pool->getStructure(
					this->nfa, equalityPar->clone());
			newStructure->addEvent(event,
					start->getForwardEdgeByIndex(i));
			break;
		}
	}
}


template<class Tin, class Tout, class Tdep>
void CEPEngine<Tin, Tout, Tdep>::setWindowConstraint(long period, int fromEvent,
		int toEvent) {
	windowConst->eventFrom = fromEvent;
	windowConst->eventTo = toEvent;
	windowConst->period = period;
	if (toEvent == -1 && fromEvent == -1)
		windowConst->window = WindowStruct::FirstLastEvents;
	else if (toEvent == -1)
		windowConst->window = WindowStruct::FromLastEvents;
	else
		windowConst->window = WindowStruct::FromToEvents;
}
}

#endif /* CEPEngine_hpp_ */
