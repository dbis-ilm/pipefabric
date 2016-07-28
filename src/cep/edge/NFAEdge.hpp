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

#ifndef NFAEdge_hpp_
#define NFAEdge_hpp_
#include <iostream>
#include <cassert>
#include <boost/function.hpp>
#include "../Instance.hpp"
using namespace std;
namespace pfabric {

/**
 * @brief an object represents an automaton edge in our engine. In general, we have two types
 * of edges, forward edges to jump from state to state and loop edges to stay in the same state as a loop
 * Each edge has associated predicate to evaluate an incoming tuple against a condition. According to the result
 * of this condition, the engine will decide to jump to the next state or not.
 */
template<class TinPtr, class ToutPtr, class TdepPtr>
class NFAStructure;
template<class TinPtr, class ToutPtr, class TdepPtr>
class NFAEdge {

public:
	/**
	 * A predicate function to evaluate an incoming tuple, it returns boolean value to indicate
	 * if the condition has been satisfied. If the condition is satisfied, the engine should store the incoming
	 * tuple and jump to the next state. If not, the engine should stay in the current state unless receiving a tuple
	 * that satisfied this condition.It takes two parameters. First, incoming tuples to evaluate
	 * and second, some related values from other edges or previous stored tuples.
	 */
	typedef boost::function<bool(const TinPtr&, const TdepPtr&)> EdgePredicate;
	typedef ns_types::SharedPtr<NFAEdge<TinPtr, ToutPtr, TdepPtr>> NFAEdgePtr; //class NFAEdge
	enum EdgeType {
		Loop, Forward
	};
	/**
	 * A default constructor to set the edge id and its predicate to evaluate the incoming tuple against
	 * a condition.
	 * @param edgeID the edge id assigned to this edge
	 * @pram predicate the predicate assigned to this edge
	 */
	NFAEdge(int edgeID, EdgePredicate predicate) :
			edgeID(edgeID), predicate(predicate) {
		assert(predicate);
	}

	/**
	 * A default constructor to set the id of the edge without assigning a predicate
	 * @param edgeaID the edge id assigned to this edge
	 */
	NFAEdge(int edgeID) :
			edgeID(edgeID) {
	}
	/**
	 * A virtual destructor
	 * nothing to do here
	 */
	virtual ~NFAEdge() {
	}
	/**
	 * Output member variable information.
	 * @param out the output stream handle.
	 */
	virtual void write(ostream& out = cout) const= 0;
	/**
	 * Get the type of this edge either loop or forward
	 * @return As above.
	 */
	const virtual EdgeType getEdgeType() const = 0;
	/**
	 * Set the predicate on this edge. The predicate associated with this edge and responsible
	 * for evaluating the incoming tuples against a condition.
	 * @param predicate The predicate to set to
	 */
	void setPredicate(EdgePredicate predicate) {
		this->predicate = predicate;
	}
	/**
	 * Get the associated predicate of this edge.
	 * @return As above.
	 */
	EdgePredicate getPredicate() const {
		return this->predicate;
	}
	/**
	 * An equality comparison operator.
	 * @param rhs the input edge for this edge to compare to
	 * @return true iff this edge has the same predicate and edge id as that of rhs
	 */
	virtual bool operator ==(const NFAEdge& rhs) const {
		if (this == &rhs)
			return true;
		return this->edgeID == rhs.getID();
	}
	/**
	 * An inequality comparison operator.
	 * @param rhs The input edge for this edge to compare to
	 * @return true iff this edge does not have the same predicate and edge id as that of rhs
	 */
	virtual bool operator !=(const NFAEdge& rhs) const {
		return !(this->operator ==(rhs));
	}
	/**
	 * Get the ID of this edge as  an integer
	 * @return as above
	 */
	int getID() const {
		return this->edgeID;
	}
	/**
	 * Set the ID of this edge
	 * @param  edge_id ID of this edge
	 */
	void setID(int edgeID) {
		this->edgeID = edgeID;
	}

	//bool evaluate(const TinPtr& tup);
	/**
	 * evaluate incoming tuple by this edge. The edge sometimes needs some related
	 * values from other edges. Therefore, the CEP structure has to be checked as well
	 * @param tup the incoming tuple
	 * @param str the CEP structure to get some related values from other edges
	 */
	bool evaluate(const TinPtr& tup,
			const boost::intrusive_ptr<NFAStructure<TinPtr, ToutPtr, TdepPtr>>& str);

protected:

	/**
	 * This variable stores the ID of this edge assigned by the engine
	 */
	int edgeID;
	/**
	 * This variable stores a pointer to the predicate which is responsible for evaluating the
	 * incoming tuple against a condition.
	 */
	EdgePredicate predicate;

};

} //

namespace pfabric {

template<class TinPtr, class ToutPtr, class TdepPtr>
bool NFAEdge<TinPtr, ToutPtr, TdepPtr>::evaluate(
		const TinPtr& tup,
		const boost::intrusive_ptr<NFAStructure<TinPtr, ToutPtr, TdepPtr>>& str) {
	if (predicate(tup, (str == NULL ? NULL : str->getRelatedValue()))) {
		return true;
	}
	return false;
}
}
#endif /* NFAEdge_hpp_ */
