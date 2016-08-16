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
template<class TinPtr, class ToutPtr, class TdepPtr>
class Matcher;
//class GCStructures;
template<class TinPtr, class ToutPtr, class TdepPtr>
class CEPEngine {
protected:
	/**
	 * CEP manager to notify the system with the matched events
	 */
	Matcher<TinPtr, ToutPtr, TdepPtr> *manager;
	/**
	 * statistical counter to store  number of matches
	 */
	long counter;
	/**
	 * a pool of structures
	 */
	StructurePool<TinPtr, ToutPtr, TdepPtr>* pool;
	/**
	 * to be deleted structures, these can be outputted structures or violated time constraint structures
	 */
	std::list<typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr> deletedStructures;
	/**
	 * the global NFA
	 */
	typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr  nfa;
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
	bool checkWindowTime(const TinPtr& event, const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str);
	/**
	 * check edge predicates
	 * @param event
	 * @param str
	 * @return
	 */
	int checkPredicate(const TinPtr& event, const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str,
			typename NFAState<TinPtr>::StateType &type);
public:
	/**
	 * constructor to receive the CEP manager
	 * @param manager
	 */
	CEPEngine(Matcher<TinPtr, ToutPtr, TdepPtr> *manager): counter(0), cgIndicator(false) {
		this->manager = manager;
		this->nfa = typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr (new NFAController<TinPtr, ToutPtr, TdepPtr>());
		this->pool = new StructurePool<TinPtr, ToutPtr, TdepPtr>();
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
	StructurePool<TinPtr, ToutPtr, TdepPtr>* getStructurePool() const {
		return this->pool;
	}
	/**
	 * set the structures pool
	 * @param pool the structures pool
	 */
	void setStructurePool(StructurePool<TinPtr, ToutPtr, TdepPtr>* pool) {
		this->pool = pool ;
	}
	/**
	 * get the structures to be deleted
	 * @return the structures to be deleted
	 */
	std::list<typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr> getDeletedStructures() const {
		return this->deletedStructures;
	}
	/**
	 *  set the structures to be deleted
	 * @param deletedStructures the structures to be deleted
	 */
	void setDeletedStructures(std::list<typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr> deletedStructures) {
		this->deletedStructures = deletedStructures;
	}
	/**
	 * get our working NFA
	 * @return  our working NFA
	 */
	const typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr  getNFA() const {
		return this->nfa;
	}
	/**
	 * set our working NFA
	 * @param nfa our working NFA
	 */
	void setNFA(typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr nfa) { this->nfa = nfa; }
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


template<class TinPtr, class ToutPtr, class Tdep>
CEPEngine<TinPtr, ToutPtr, Tdep>::~CEPEngine() {
	delete this->pool;
	delete this->equalityPar;
	delete this->windowConst;
}

template<class TinPtr, class ToutPtr, class TdepPtr>
bool CEPEngine<TinPtr, ToutPtr, TdepPtr>::checkWindowTime(const TinPtr& event,
		const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str) {
/*
	if (windowConst->window == WindowStruct::FirstLastEvents) {
		if (event->timestamp() - str->getFirstEventTimestamp()
				<= windowConst->period) {
			return true;
		}
	} else if (windowConst->window == WindowStruct::FromLastEvents) {
		if (event->timestamp()
				- str->getEventTimestamp(windowConst->eventFrom)
				<= windowConst->period)
			return true;
	} else if (windowConst->window == WindowStruct::FromToEvents) {
		if (str->getEventTimestamp(windowConst->eventTo)
				- str->getEventTimestamp(windowConst->eventFrom)
				<= windowConst->period)
			return true;
	}
*/
	return true;
}
template<class TinPtr, class ToutPtr, class TdepPtr>
int CEPEngine<TinPtr, ToutPtr, TdepPtr>::checkPredicate(const TinPtr& event,
		const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str, typename NFAState<TinPtr>::StateType &type) {

	if (str->getCurrentState()->getStateType() == NFAState<TinPtr>::Normal) {
		//creat_new_structure(event, current_state);
		auto current = (NormalState<TinPtr, ToutPtr, TdepPtr>*)str->getCurrentState();
		for (int i = 0; i < current->getNumEdges(); i++) {
			if (current->getForwardEdgeByIndex(i)->evaluate(event, str)) {
				return i;
			}
		}
	} else if (str->getCurrentState()->getStateType()
			== NFAState<TinPtr>::Kleene) {

		//try go to the next
		auto current = (KleeneState<TinPtr, ToutPtr, TdepPtr>*)(
				str->getCurrentState());
		bool forwardOK = false;
		switch (current->getSpecification()) {
		case KleeneState<TinPtr, ToutPtr, TdepPtr>::Star:
		case KleeneState<TinPtr, ToutPtr, TdepPtr>::Question: {
			forwardOK = true;
			break;
		}
		case KleeneState<TinPtr, ToutPtr, TdepPtr>::Plus: {
			if (str->getCurrentKleene(current) == 1) {
				forwardOK = true;
			}
			break;
		}
		case KleeneState<TinPtr, ToutPtr, TdepPtr>::Restricted:
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
			case KleeneState<TinPtr, ToutPtr, TdepPtr>::Star:
			case KleeneState<TinPtr, ToutPtr, TdepPtr>::Question: {
				str->addEvent(event, loop);
				break;
			}
			case KleeneState<TinPtr, ToutPtr, TdepPtr>::Plus: {
				if (str->getCurrentKleene(current) == 0) {
					str->addEvent(event, loop);
				}
				break;
			}
			case KleeneState<TinPtr, ToutPtr, TdepPtr>::Restricted: {
				if (str->getCurrentKleene(current)
						< current->getLoopEdge()->getNumOfLoop()) {
					str->addEvent(event, loop);
				}
				break;
			}
			}
		}
	} else if (str->getCurrentState()->getStateType()
			== NFAState<TinPtr>::Negation) {
		auto current = (NormalState<TinPtr, ToutPtr, TdepPtr>*)str->getCurrentState();
		do {
			for (int i = 0; i < current->getNumEdges(); i++) {
				if (current->getForwardEdgeByIndex(i)->evaluate(event,
						str)) {
					type = NFAState<TinPtr>::Negation;
					return -1;
				}
			}
			current = (NormalState<TinPtr, ToutPtr, TdepPtr>*)(
					current->getForwardEdgeByIndex(0)->getDestState());
		} while (current->getStateType() == NFAState<TinPtr>::Negation);
		if (current->getStateType() == NFAState<TinPtr>::Final)
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

template<class TinPtr, class ToutPtr, class Tdep>
void CEPEngine<TinPtr, ToutPtr, Tdep>:: runGCstructures() {
	for (int i = 0; i < this->deletedStructures.size(); i++) {
		const typename NFAStructure<TinPtr, ToutPtr, Tdep>::NFAStructurePtr& str = this->deletedStructures.front();
		auto par = str->getEqualityValue();
		typename ValueIDMultimap<typename NFAStructure<TinPtr, ToutPtr, Tdep>::NFAStructurePtr, TinPtr>::MultimapPair iterPair =
				this->pool->getValue(par);

		typename ValueIDMultimap<typename NFAStructure<TinPtr, ToutPtr, Tdep>::NFAStructurePtr, TinPtr>::MultimapConstIterator it = iterPair.first;
		for (; it != iterPair.second; ++it) {
			if (it->second == str) {
				pool->removeValue(it);
				break;
			}
		}
		this->deletedStructures.pop_front();
	}
}
template<class TinPtr, class ToutPtr, class Tdep>
void CEPEngine<TinPtr, ToutPtr, Tdep>::createStartStructure(const TinPtr& event) {
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


template<class TinPtr, class ToutPtr, class Tdep>
void CEPEngine<TinPtr, ToutPtr, Tdep>::setWindowConstraint(long period, int fromEvent,
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
