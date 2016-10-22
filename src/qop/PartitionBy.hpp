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

#ifndef PartitionBy_hpp_
#define PartitionBy_hpp_

#include <boost/unordered/unordered_map.hpp>
#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/UnaryTransform.hpp"
#include "qop/Queue.hpp"
#include "qop/DataSink.hpp"

namespace pfabric {


/**
 * \brief an operator for partitioning the input stream and running subqueries in separate threads on each partition
 *
 * The PartitionSplit operator partitions the input stream on given partition id which is derived using a user-defined function
 * and forwards the tuples of each partition to a subquery which is constructed by the given QueryBuilder function.
 */
template<typename StreamElement>
class PartitionBy: public UnaryTransform<StreamElement, StreamElement> {
public:
	PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

	typedef DataSource<StreamElement> SourceBase;
	typedef std::shared_ptr<SourceBase> SourcePtr;

	typedef DataSink<StreamElement> SinkBase;
	typedef std::shared_ptr<SinkBase> SinkPtr;

	/**
	 * Typedef for the partition id.
	 */
	typedef std::size_t PartitionID;

	/**
	 * Typedef for a function pointer to a partition function.
	 */
	typedef std::function<PartitionID(const StreamElement&)> PartitionFunc;

	/**
	 * Creates a new instance of the operator.
	 *
	 * \param pFun the function for deriving the partition id
	 * \param qFun the function for constructing the subquery
	 * \param mergeOp an operator instance for merging the results of the subquery and acting as the publisher for subsequent operators
	 * \param numPartitions the number of partitions, if numPartitions == 0 then partitions are created on demand
	 */
	PartitionBy(unsigned int numPartitions, PartitionFunc pFun = nullptr) :
			mNumPartitions(numPartitions), mFunc(pFun) {
	}

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, PartitionBy, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, PartitionBy, processPunctuation );

	/**
	 * This method is invoked when a punctuation arrives. It simply forwards the punctuation
	 * to the subscribers.
	 *
	 * \param punctuation the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		// TODO
		this->getOutputPunctuationChannel().publish(punctuation);
	}

	/**
	 * This method is invoked when a tuple arrives from the publisher. It forwards the incoming tuple
	 * to the corresponding partition.
	 *
	 * \param data the incoming tuple
	 * \param c the channel at which we receive the tuple
	 * \param outdated indicates whether the tuple is new or invalidated now (outdated == true)
	 */
	void processDataElement(const StreamElement& data, const bool outdated) {
		PartitionID keyVal = mFunc(data);
		typename PartitionTable::iterator it = mPartitions.find(keyVal);
		if (it != mPartitions.end()) {
			auto qop = it->second;
			// we cannot simply call our publish method, because we want to inform
			// only the operator child - all other subscribers are notified
			// by their own thread
			qop->template getOutputChannelByID<0>().publish(data, outdated);
		}
	}

	/**
	 * Creates a new subquery using the QueryBuilder function, connects its operators
	 * accordingly, and registers it in the partition table.
	 *
	 * \param key the partition key
	 */
	void setSubscriberForPartitionID(PartitionID id, SinkPtr subscriber) {
		BOOST_ASSERT_MSG(id < mNumPartitions, "invalid partition id");

		// first, we create a SourceBase as the root of the subquery
		// because we need an operator whose OutputChannel can be accessed
		// to send tuples
		auto src = std::make_shared<SourceBase>();

		// then we decouple the subquery into a separate thread by introducing a Queue
		auto queue = std::make_shared<Queue<StreamElement>>();
		CREATE_LINK(src, queue);

		// both operators are interconnected
		CREATE_LINK(queue, subscriber); // <-- TODO
		mPartitions.insert(std::make_pair(id, src));
	}

protected:

	/**
	 * Typedef for the table mapping keys to partitions.
	 */
	typedef boost::unordered_map<PartitionID, SourcePtr> PartitionTable;

	PartitionTable mPartitions;  //< a hashtable for mapping parition ids to subqueries
	unsigned int mNumPartitions; //< maximum number of partitions (subqueries) to be created
	PartitionFunc mFunc;         //< pointer to the function producing the partition id
};

}

#endif
