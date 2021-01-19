/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */



#ifndef StructurePool_hpp_
#define StructurePool_hpp_
#include "NFAStructure.hpp"
#include "util/ValueIDMultimap.hpp"
#include "NFAController.hpp"
/**
 * @brief A structures pool to keep all lived structures (sequences) inside. It is responsible
 * for creating the sequences and put them inside a multi-map to retrieve them as fast as possible. In other words, It is a multi-map container with a partition pointer key and
 * sequence pointer as value. Partitioning technique is used to speed up the lookup to all current
 * lived structures. To detect the complex event, then engine should go through this pool (or a partition "part") and process these sequences.
 */
namespace pfabric {
template<class TinPtr, class ToutPtr, class TdepPtr>
class StructurePool: public ValueIDMultimap<
		typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr, TinPtr> {
	typedef boost::shared_ptr<StructurePool<TinPtr, ToutPtr, TdepPtr>> StructurePoolPtr;
public:
	/**
	 * A default constructor
	 * Nothing to do
	 */
	StructurePool() {}
	/**
	 * A virtual destructor
	 * Nothing to do
	 */
	virtual ~StructurePool() {}
	/**
	 * Get a structure from this pool after creating it. It receive its partition and a pointer to the original
	 * NFA. Once this function creates a new sequence, it puts the sequence inside a multi-map
	 * by using appendValue. It also lets the sequence to know its partition in this pool.
	 * @param nfa the original NFA for the system which is shared among all running sequences
	 * @param p the partition of this sequence in the pool
	 * @return a created sequence from this pool
	 */
	const typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr getStructure(
			const typename NFAController<TinPtr, ToutPtr, TdepPtr>::NFAControllerPtr& nfa,
			Partition<TinPtr>* p) {
		typename NFAStructure<TinPtr, ToutPtr, TdepPtr>::NFAStructurePtr str(
				new NFAStructure<TinPtr, ToutPtr, TdepPtr>(nfa));
		str->setEqualityValue(p);
		this->appendValue(p, str);
		return str;
	}
	/**
	 * Output the information  into ostream object
	 * @param out The output stream handle.
	 */
	void print(std::ostream& out = std::cout) const {
		/*
		 os << std::endl;
		 for (value_id_multimap<
		 structure_ptr>::multimap_iterator it =
		 this->value_id.begin(); it != this->value_id.end(); it++) {
		 //os << " [" << it->first << ", ";
		 //it->second->print(os);
		 os << "]";
		 }
		 os << std::endl;*/
	}

	/**
	 * Remove all structure except the structures which have this id or more
	 * @param str the structure id to remove all structure above it
	 */
	void removeAllExceptMore(long str) {
		/*
		 for (value_id_map<structure_ptr>::map_const_iterator it =
		 this->value_id.begin(); it != this->value_id.end();) {
		 if (it->first < str) {
		 this->value_id.erase(it++);
		 } else {
		 ++it;
		 }
		 }
		 */
	}

};

}
#endif /* StructurePool_hpp_ */
