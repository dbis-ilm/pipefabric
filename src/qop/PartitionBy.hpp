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

#ifndef PartitionBy_hpp_
#define PartitionBy_hpp_

#include <boost/unordered/unordered_map.hpp>

#include <utility>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/UnaryTransform.hpp"
#include "qop/Queue.hpp"

namespace pfabric {


/**
 * @brief PartitionBy is an operator for partitioning the input stream and
 * running subqueries in separate threads on each partition.
 *
 * The PartitionBy operator partitions the input stream on given partition id
 * which is derived using a user-defined function and forwards the tuples of
 * each partition to a subquery. Subqueries are registered via their input channels
 * for each partition id.
 *
 * @tparam StreamElement
 *   the data stream element type consumed by PartitionBy
 */
template<typename StreamElement>
class PartitionBy : public UnaryTransform<StreamElement, StreamElement> {
public:
	PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

	/**
	 * Typedef for the partition id.
	 */
	typedef std::size_t PartitionID;

	/**
	 * Typedef for a function pointer to a partition function.
	 */
	typedef std::function<PartitionID(const StreamElement&)> PartitionFunc;

	/**
	 * Create a new instance of the operator.
	 *
	 * @param pFun the function for deriving the partition id
	 * @param numPartitions the number of partitions
	 */
	PartitionBy(PartitionFunc pFun, unsigned int numPartitions) :
			mFunc(pFun), mNumPartitions(numPartitions) {
	}

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, PartitionBy, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, PartitionBy, processPunctuation );

	const std::string opName() const override { return std::string("PartitionBy"); }

	/**
	 * This method is invoked when a punctuation arrives. It simply forwards the punctuation
	 * to all partitions.
	 *
	 * @param punctuation the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		for (auto it : mPartitions) {
      //std::cout << "PartitionBy::processPunctuation" << std::endl;
			auto qop = it.second;
			auto slot = qop->template getInputChannelByID<1>().getSlot();
			slot(punctuation);
		}
	}

	/**
	 * This method is invoked when a tuple arrives from the publisher. It forwards the incoming tuple
	 * to the corresponding partition.
	 *
	 * @param data the incoming tuple
	 * @param outdated indicates whether the tuple is new or invalidated now (outdated == true)
	 */
	void processDataElement(const StreamElement& data, const bool outdated) {
		// first, we determine the partition id
		PartitionID keyVal = mFunc(data);
		typename PartitionTable::iterator it = mPartitions.find(keyVal);
		if (it != mPartitions.end()) {
			auto qop = it->second;
			auto slot = qop->template getInputChannelByID<0>().getSlot();
 			slot(data, outdated);
		}
	}

	/**
	 * Register an operator (i.e. its data and punctuation channel for a given
   * partition id). All stream elements whose partition id (determined by the
   * partitioning functio)n is equal to the given id are forwarded to this
	 * data channel. Punctuations are always sent to all partitions.
	 *
	 * @param id the partition id
   * @param dataChannel the input data channel of the operator associated with this partition
	 * @param	punctuationChannel the input punctuation channel of the operator
	 */
  template <typename DataChannel, typename PunctuationChannel>
	void connectChannelsForPartition(PartitionID id, DataChannel& dataChannel,
				PunctuationChannel& punctuationChannel) {
		BOOST_ASSERT_MSG(id >= 0 && id < mNumPartitions, "invalid partition id");
		// we decouple the channels by introducing a Queue operator which
		// runs the consumer side within a separate thread
		auto queue = std::make_shared<Queue<StreamElement> >();

		// and connect the Queue to the given channels
		connectChannels(queue->getOutputDataChannel(), dataChannel);
		connectChannels(queue->getOutputPunctuationChannel(), punctuationChannel);

		// finally, we register the Queue in our hashtable
		mPartitions.insert({ id, queue });
	}

protected:

	/**
	 * Typedef for the table mapping keys to partitions, i.e. the Queue instance.
	 */
	typedef std::shared_ptr<Queue<StreamElement> > QueuePtr;
	typedef boost::unordered_map<PartitionID, QueuePtr> PartitionTable;

	PartitionTable mPartitions;  //< a hashtable for mapping parition ids to Queue
	PartitionFunc mFunc;         //< pointer to the function producing the partition id
	unsigned int mNumPartitions; //< number of partitions
};

}

#endif
