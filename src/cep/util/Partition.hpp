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

#ifndef Partition_hpp_
#define Partition_hpp_

#include "core/Tuple.hpp"

namespace pfabric {
template <class TinPtr>
struct Partition {
	enum PartitionType {
		Sequence, Attribute
	};
	virtual ~Partition() {}
	virtual size_t hashSelf() const = 0;
	virtual bool equal(const Partition &) const = 0;
	virtual void generateValues(const TinPtr& event) = 0;
	virtual Partition* clone() = 0;
	virtual PartitionType getType() = 0;
	virtual void print() {
	}
};
//template <class TinPtr>
//typedef boost::shared_ptr<Partition<TinPtr>> Partition_ptr;
template <class TinPtr>
struct hash_function {
	size_t operator()(const Partition<TinPtr>* p) const {
		return p->hashSelf();
	}
};
template <class TinPtr>
struct equal_function {
	bool operator()(const Partition<TinPtr>* p, const Partition<TinPtr>* q) const {
		return p->equal(*q);
	}
};
template <class TinPtr>
struct SequencePartition: public Partition<TinPtr> {
	static int sequence;
	int value;
	inline bool operator==(const SequencePartition & other) const {
		return value == other.value;
	}
	size_t hashSelf() const {
		return value;

	}
	bool equal(const Partition<TinPtr> & p) const {
		return *this == static_cast<const SequencePartition &>(p);
	}
	void generateValues(const TinPtr& event) {
		value = sequence++;
	}
	typename Partition<TinPtr>::PartitionType getType() {
		return Partition<TinPtr>::Sequence;
	}
	Partition<TinPtr>* clone() {
		SequencePartition* seq = new SequencePartition;
		seq->value = value;
		return seq;
	}
	virtual void print() {
		//std::cout << x << " " << y << " " << z << std::endl;
	}

};
template <class TinPtr>
int SequencePartition<TinPtr>::sequence = 0;
/*
struct AttriburePartition: public Partition {
	int x;
	static int index;
	inline bool operator==(const AttriburePartition & other) const {
		return x == other.x;
	}
	virtual ~AttriburePartition() {
	}
	size_t hash_self() const {
		std::size_t hash = 0;
		boost::hash_combine(hash, x);
		return hash;

	}
	bool equal(const Partition & p) const {
		return *this == static_cast<const AttriburePartition &>(p);
	}
	void generate_values(const tuple_ptr& event) {
		x = event->to_int(index);
	}

	Partition::type get_type() {
		return Partition::ATTRIBUTE;
	}
	Partition* clone() {
		AttriburePartition* par = new AttriburePartition;
		par->x = x;
		return par;
	}
	void set_Partition_index(int idx) {
		index = idx;
	}
	virtual void print() {
	}

};
*/
/*
struct AttributePartition: public Partition {
	static int index ;
	int value1, value2, value3 ;
	inline bool operator==(const AttributePartition & other) const {
		return value1 == other.value1  &&   value2 == other.value2 &&  value3 == other.value3  ;
	}
	virtual ~AttributePartition() {
	}
	size_t hashSelf() const {
		std::size_t hash = 0;
		boost::hash_combine(hash,value1) ;
		boost::hash_combine(hash,value2) ;
		boost::hash_combine(hash,value3) ;
		return hash ;

	}
	bool equal(const Partition & p) const {
		return *this == static_cast<const AttributePartition &>(p);
	}
	void generateValues(const tuple_ptr& event) {
		value1 = event->to_int(2);
		value2 = event->to_int(6) ;
		value3 = event->to_int(4);
	}
	Partition* clone()  {
		AttributePartition* par  = new AttributePartition;
		par->value1 = value1 ;
		par->value2 = value2 ;
		par->value3 = value3 ;
		return par;
	}
	Partition::PartitionType getType() {
			return Partition::Attribute;
		}
	void setPartitionIndex(int idx) {
		index = idx ;
	}
	virtual void print() {
	}

};

template<typename T>
struct PtrDeleter: public std::unary_function<bool, T> {
	bool operator()(T* ptr) const {
		delete ptr;
		return true;
	}
};*/
}
#endif /* Partition_hpp_ */
