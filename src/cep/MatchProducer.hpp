/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
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

#ifndef MatchProducer_hpp_
#define MatchProducer_hpp_
#include "Instance.hpp"
#include "NFAStructure.hpp"
//#include "EventBuffer.hpp"
/**
 * A class to produce a match once the engine reaches the final state. The result can be multiple
 * complex events since we have multiple structures running at the same time.
 */

namespace pfabric {
template<class TinPtr, class ToutPtr, class TdepPtr>
class MatchProducer {
public:
	typedef boost::shared_ptr<MatchProducer<TinPtr, ToutPtr, TdepPtr>> MatchProducerPtr;
	typedef typename std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr> matchesList;
	typedef typename std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr>::const_iterator matchConstIterator;
	/**
	 * A constructor to receive and build a match from a 'structure' which has events ids for the match, then get the actual events
	 * from an event buffer to build a match
	 * @param run the structure which has the ids of events
	 * @param buffer the buffer which has the actual events
	 */
	MatchProducer() {
	}
	/**
	 * generate a new tuple which has all corresponding events
	 * @return a new tuple which has all corresponding events
	 */
	ToutPtr produceTogether(
			const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str) {
		//int index = 0;
		typename MatchProducer<TinPtr, ToutPtr, TdepPtr>::matchesList matches;

		std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr> l =
				str->getEvents();
		for (typename std::vector<typename Instance<TinPtr, ToutPtr>::InstancePtr>::const_iterator i =
				l.begin(); i != l.end(); i++) {
			auto tup = (*i)->getOriginalEvent();
			for (int j = 0; j < tup->size(); j++) {
				//data.at(index++) = (tup->operator [](j));
			}
			//data.at(index++) = (*i)->getState();
			//data.at(index++) = (*i)->getSequenceInComplex();
		}
		//return ToutPtr(new Tout(/*data*/)); // create new tuple which combine all the matching events
	}

	/**
	 * get the list of all matches for this structure
	 * @return a list of all matches for this structure
	 */
	matchesList produceAsList(
			const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str) {
		return str->getEvents();
	}

	/**
	 * number of matches in this structure
	 * @return the size
	 */
	int sizeOfMatch(
			const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr& str) {
		return str->getSequence();
	}
	/**
	 * destructor
	 */
	virtual ~MatchProducer() {
	}
};

}
#endif /* MatchProducer_H_ */
