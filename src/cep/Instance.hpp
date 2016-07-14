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
#ifndef Instance_hpp_
#define Instance_hpp_
#include <iostream>
#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/quick_allocator.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

#include "core/Tuple.hpp"

using namespace std;
using namespace boost::posix_time;
namespace pfabric {
/**
 * @brief An Instance class which presents the  CEP event to be stored in the system and outputted later
 * Hence, this class will add the state name (when this event matches a particular state in the NFA ) and the event sequence to the original tuple
 * Sequence is necessary to mark the order of this event in the complex event.
 */
//class Instance;
template<class Tin, class Tout>
class Instance {
	typedef boost::intrusive_ptr<Tin> TinPtr;
	typedef boost::intrusive_ptr<Tout> ToutPtr;
	typedef pfabric::Tuple<std::string, int> FooterTuple;
	typedef boost::intrusive_ptr<FooterTuple> FooterTuplePtr;
private:
	/**
	 * Reference counter for intrusive pointer as a means for coordinating multiple threads
	 *
	 */
	mutable boost::atomic<int> refCount;
	/**
	 * The state name in which original event matches a particular state
	 */
	std::string state;
	/**
	 * Instance sequence in complex event necessary for repetition state
	 */
	int sequenceInComplex;
	/**
	 * The original tuple(event) to be stored
	 */
	TinPtr originalEvent;

public:

	typedef boost::intrusive_ptr<Instance<Tin, Tout>> InstancePtr;
	/**
	 * A constructor: receive the original CEP event and map it to an instance
	 * @param event the original CEP event
	 */
	Instance(TinPtr event) :
			refCount(0) {
		assert(event);
		this->originalEvent = event;
		this->state = "";
		this->sequenceInComplex = 0;
	}
	/**
	 * Virtual destructor
	 */
	virtual ~Instance() {
	}
	/**
	 * Get the original tuple (event) from this instance
	 * @return as above
	 */
	TinPtr getOriginalEvent() const {
		return this->originalEvent;
	}
	/**
	 * Set the original tuple (event) for this instance
	 * @param as above
	 */
	void setOriginalEvent(TinPtr originalEvent) {
		this->originalEvent = originalEvent;
	}
	/**
	 * Set the sequence of this event in the complex event
	 * @return as above
	 */
	int getSequenceInComplex() const {
		return this->sequenceInComplex;
	}
	/**
	 * Set the sequence of this event in the complex event
	 * @param as above
	 *
	 */
	void setSequenceInComplex(int sequenceInComplex) {
		this->sequenceInComplex = sequenceInComplex;
	}
	/**
	 * Get the state name where this the original event matches a particular state
	 * @return the state name
	 */
	std::string getState() const {
		return this->state;
	}
	/**
	 * Set the name
	 * @param state the state name
	 */
	void setState(std::string state) {
		this->state = state;
	}
	/**
	 * Convert this Instance to a normal tuple for further processing since PipeFabric deals with tuples
	 * not events.
	 * @return tuple construction
	 */
	ToutPtr convertInstanceToTuple() {
		/**
		 * A tuple contains the sate name and the sequence should be combined with the original
		 * event data. The timestamp of this instance is the timestamp of the original event
		 */
		FooterTuplePtr foot = makeTuplePtr(state, sequenceInComplex);
		ToutPtr res(
				new Tout(std::tuple_cat(originalEvent->data(), foot->data())));
		res->setTimestamp(originalEvent->getTimestamp());
		return res;
	}

	/**
	 * get the timestamp for this Instance, it is equal to  the timestamp of the original tuple
	 * timestamp
	 * @retun as above
	 */
	Timestamp getInstanceTimestamp() {
		return this->originalEvent->getTimestamp();
	}

	/**
	 * Output member variable information.
	 * @param os the output stream handle.
	 */
	void print(ostream& os = cout) {
		os << "[ " << this->sequenceInComplex << "," << this->state << " ,";
		//this->originalEvent->print(os);
		os << " ]\n";
	}

	/**
	 * The total size of this instance (the size of original tuple + 2);
	 * retrun as above
	 */
	int size() {
		return originalEvent->size() + 2;
	}

	/**
	 * Get the timestamp for this Instance as ptime, it is equal to original tuple
	 * timestamp
	 * @return as above
	 */
	ptime getInstanceTimestampAsPtime();
//	friend void intrusive_ptr_add_ref(Instance * p) {
//		p->ref_count++;
//	}
//	friend void intrusive_ptr_release(Instance * p) {
//		if (--p->ref_count == 0) {
//			delete p;
//		}
//	}
	/**
	 * The intrusive_ptr class stores a pointer to an object with
	 * an embedded reference count (refCount).
	 * This function increments the reference count by one
	 *
	 */
	friend void intrusive_ptr_add_ref(const Instance *x) {
		x->refCount.fetch_add(1, boost::memory_order_relaxed);
	}
	/**
	 *  This function is responsible for destroying the instance object
	 *  when its reference count reaches zero
	 */
	friend void intrusive_ptr_release(const Instance *x) {
		if (x->refCount.fetch_sub(1, boost::memory_order_release) == 1) {
			boost::atomic_thread_fence(boost::memory_order_acquire);
			delete x;
		}
	}
};

} /* namespace pfabric */
#endif /* Instance_hpp_ */
