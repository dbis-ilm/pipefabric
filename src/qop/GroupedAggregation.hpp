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
#ifndef GroupedAggregation_hpp_
#define GroupedAggregation_hpp_

#include "qop/AggregateStateBase.hpp"
#include "qop/AggregateFunctions.hpp"
#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TriggerNotifier.hpp"

#include <boost/core/ignore_unused.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/unordered/unordered_map.hpp>


namespace pfabric {

/**
 * @brief A grouped aggregation operator for streams of tuples.
 *
 * This operator implements the calculation of grouped aggregates over a data streams.
 * For each incoming tuple the group is determined by its key and the corresponding
 * aggregates are computed incrementally using the IterateFunc function. The final aggregation
 * results calculated by an FinalFunc function are either produced periodically or at the end
 * of the stream. The temporal behaviour is defined by the trigger type (all, timestamp, count - see
 * PipeFabricTypes.hpp)  and the trigger interval.
 *
 * @tparam InputStreamElement
 *    the data stream element type consumed by the aggregation
 * @tparam OutputStreamElement
 *    the data stream element type produced by the aggregation
 * @tparam AggregateState
 *    an element type to maintain the state of a single aggregation tuple that is used
 *    to construct the OutputStreamElement
 * @tparam KeyType
 *    the data type for the key column
 */
template<
	typename InputStreamElement,
	typename OutputStreamElement,
  typename AggregateState,
  typename KeyType = DefaultKeyType
>
class GroupedAggregation :
  public UnaryTransform< InputStreamElement, OutputStreamElement > {
	public:
		/// a pointer to an aggregation state
		typedef std::shared_ptr<AggregateState> AggregateStatePtr;
protected:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement);

	/// the function for extracting the timestamp from a tuple
	typedef std::function<Timestamp(const InputStreamElement&)> TimestampExtractorFunc;

	/// the type for the hash table to store group keys + aggregate states
	typedef boost::unordered_map< KeyType, AggregateStatePtr > HashTable;

	/// the function for calculating a grouping key for an incoming stream element
	typedef std::function< KeyType(const InputStreamElement&) > GroupByFunc;

	/**
	 * The type returned by the final aggregation function.
	 *
	 * Even if this operator publishes aggregation result elements as reference, the
	 * user-defined aggregation function must never return a dangling reference from
	 * its local scope. Its result elements are accepted by value.
	 */
	typedef typename std::remove_reference< OutputStreamElement >::type FinalAggregationResult;

public:
	/**
	 * @brief The aggregation function which produces the final (or periodic) aggregation result.
	 *
	 * This function gets a pointer to the aggregate state as well as the timestamp for the result element.
	 */
	typedef std::function< FinalAggregationResult(AggregateStatePtr) > FinalFunc;

	/**
	 * @brief The function which is invoked for each incoming stream element to calculate the incremental aggregates.
	 *
	 * This function gets the incoming stream element, the aggregate state, and the boolean flag for outdated elements.
	 */
	typedef std::function< void(const InputStreamElement&, AggregateStatePtr, const bool)> IterateFunc;

protected:
	/// a mutex for protecting aggregation processing from concurrent sources
	typedef boost::mutex AggregationMutex;

	/// a scoped lock for the mutex
	typedef boost::lock_guard< AggregationMutex > Lock;


public:

/**
	* @brief Create a new instance of the GroupedAggregation operator.
	*
	* Create a new instance of the operator for computing aggregates per groups.
	* The behaviour is defined by the trigger type (all, timestamp, count - see PipeFabricTypes.hpp)
	* and the trigger interval.
	*
	* @param groupby_fun
	*    a function pointer for getting the group id
	* @param final_fun
	*    a function pointer to the aggregation function
	* @param it_fun
	*    a function pointer to an iteration function called for each incoming tuple
	* @param tType
	*    the trigger type specifying when an aggregation tuple is produced
	*    (TriggerAll = for each incoming tuple,
	*     TriggerByCount = as soon as a number of tuples (tInterval) are processed,
	*     TriggerByTime = after every tInterval seconds,
	*     TriggerByTimestamp = as for TriggerByTime but based on timestamp of the tuples
	*                          and not based on real time)
	* @param tInterval
	*    the time interval in seconds to produce aggregation tuples (for trigger by timestamp)
	*    or in the number of tuples (for trigger by count)
	*/
	GroupedAggregation(GroupByFunc groupby_fun,
					FinalFunc final_fun,
					IterateFunc it_fun,
					AggregationTriggerType tType = TriggerAll,
					const unsigned int tInterval = 0)  :
		mGroupByFunc(groupby_fun),
		mIterateFunc(it_fun), mFinalFunc(final_fun),
    mTriggerInterval( tInterval ),
    mNotifier(tInterval > 0 && tType == TriggerByTime ?
             new TriggerNotifier(std::bind(&GroupedAggregation::notificationCallback, this), tInterval) : nullptr),
    mLastTriggerTime(0), mTriggerType(tType), mCounter(0) {
	}

	/**
		* @brief Create a new instance of the GroupedAggregation operator.
		*
		* Create a new instance of the operator for computing aggregates per groups.
		* The behaviour is defined by the TriggerByTimestamp strategy.
		*
		* @param groupby_fun
		*    a function pointer for getting the group id
		* @param final_fun
		*    a function pointer to the aggregation function
		* @param it_fun
		*    a function pointer to an iteration function called for each incoming tuple
		* @param func
		*    a function for extracting the timestamp value from the stream element
 		* @param tInterval
		*    the time interval in seconds to produce aggregation tuples (for trigger by timestamp)
		*/
		GroupedAggregation(GroupByFunc groupby_fun,
						FinalFunc final_fun,
						IterateFunc it_fun,
						TimestampExtractorFunc func,
						AggregationTriggerType tType = TriggerAll,
						const unsigned int tInterval = 0)  :
			mGroupByFunc(groupby_fun),
			mIterateFunc(it_fun), mFinalFunc(final_fun),
			mTimestampExtractor(func),
	    mTriggerInterval( tInterval ),
	    mNotifier(nullptr),
	    mLastTriggerTime(0), mTriggerType(TriggerByTimestamp), mCounter(0) {
		}

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, GroupedAggregation, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, GroupedAggregation, processPunctuation );


private:

	////////////   channel callbacks   ////////////

	/**
	 * This method is invoked when a data stream element arrives.
	 *
	 * @param[in] data
	 *    the incoming stream element
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement(const InputStreamElement& data, const bool outdated) {
		Lock lock(mAggrMtx);

		const KeyType grpKey = mGroupByFunc(data);
		bool newGroup = mAggregateTable.count(grpKey) == 0;

		if(newGroup) {
			// case 1: we don't have a group yet for this key -> create a new one
      if(!outdated) {
				processNewAggregationGroup(grpKey, data, lock);
			}
      // ... and we ignore outdated tuples
    }
		else {
			// case 2: we already have got a group for this key -> update its aggregates
			updateAggregationGroup(grpKey, data, outdated, lock);
		}

    switch (mTriggerType) {
      case TriggerByCount:
      {
        if (++mCounter == mTriggerInterval) {
          notificationCallback();
          mCounter = 0;
        }
        break;
      }
      case TriggerByTimestamp:
      {
        auto ts = mTimestampExtractor(data);
        if (ts - mLastTriggerTime >= mTriggerInterval) {
          notificationCallback();
          mLastTriggerTime = ts;
        }
        break;
      }
      default:
        // TriggerAll is handled in updateAggregationGroup
        // TriggerByTime is handled by notifier thread
        break;
    }

	}

	/**
	 * @brief This method is invoked when a punctuation arrives.
	 *
	 * Punctuation tuples can trigger aggregation results if specified for the operator
	 * via the punctuation mask.
	 *
	 * @param[in] punctuation
	 *    the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
			Lock lock( mAggrMtx );
			this->getOutputPunctuationChannel().publish(punctuation);
	}


	////////////   internal helper methods   ////////////

	/**
	 * @brief Handle a data stream element for a new group.
	 *
	 * This internal helper method will generate a new aggregation state for the group
	 * and add it to the group state table. Further, the aggregation result for the
	 * new element is published if no sliding window is implemented by this operator.
	 *
	 * @param[in] grpKey
	 *    the key of the new group
	 * @param[in] data
	 *    the new data stream element
	 * @param[in] lock
	 *    a reference to the lock protecting the aggregation state
	 */
	void processNewAggregationGroup(KeyType grpKey, const InputStreamElement& data, const Lock& lock) {
		const bool outdated = false;
    const Timestamp elementTime = mTimestampExtractor != nullptr ? mTimestampExtractor(data) : 0;

		// create a new aggregation state
		AggregateStatePtr newAggrState = std::make_shared<AggregateState>();
		newAggrState->setTimestamp(elementTime);

		// ... call the iterate function
		mIterateFunc(data, newAggrState, outdated);
		// ... and insert it into the hashtable
		mAggregateTable.insert({ grpKey, newAggrState });

		// directly publish the new aggregation result if no sliding window was specified
		if (mTriggerType == TriggerAll) {
			produceAggregate(newAggrState, elementTime, outdated, lock);
		}
	}

	/**
	 * @brief Handle a data stream element for an existing group.
	 *
	 * This internal helper method will update the aggregation state for the group
	 * Further, the aggregation result for the new element is published if no sliding
	 * window is implemented by this operator.
	 * If an @c outdated @c data element is processed, the entire group might be removed
	 * if no data elements belong to it any longer.
	 *
	 * @param[in] grpKey
	 *    the key of the new group
	 * @param[in] data
	 *    the new data stream element
	 * @param[in] outdated
	 *    a flag indicating if the incoming data element was marked as outdated
	 * @param[in] lock
	 *    a reference to the lock protecting the aggregation state
	 */
    void updateAggregationGroup(const KeyType& grpKey, const InputStreamElement& data,
                                const bool outdated, const Lock& lock) {
      auto groupEntry = mAggregateTable.find(grpKey);

      AggregateStatePtr aggrState = groupEntry->second;
      const Timestamp elementTime = mTimestampExtractor != nullptr ? mTimestampExtractor(data) : 0;
      /* 1. Send the previous aggregated state as outdated
      if (mTriggerType == TriggerAll) {
        produceAggregate(aggrState, elementTime, true, lock);
      }
			*/
      // 2. update the group state (counting algorithm)
      aggrState->setTimestamp(elementTime);
      aggrState->updateCounter(outdated ? -1 : 1);
      const bool outdatedAggregate = (aggrState->getCounter() == 0);
      mIterateFunc(data, aggrState, outdated);

      // 3. directly publish the new aggregation result if no sliding window was specified
      // TODO: should an outdated tuple trigger also an aggregation tuple??
      if (mTriggerType == TriggerAll) {
        produceAggregate(aggrState, elementTime, outdated, lock);
      }

      // 4. purge the aggregate if the group vanishes
      if (outdatedAggregate) {
        // we remove the entry from the hashtable if all tuples belonging to this
        // aggregate are outdated - counting algorithm
        mAggregateTable.erase(groupEntry);
      }
  }

	/**
	 * @brief Produce a final aggregate for a specific state and publish it to all subscribers.
	 *
	 * This method applies the user-defined aggregation finalization operation to the
	 * aggregation @c state in order to obtain a final aggregation result. This result
	 * is forwarded to all subscribing components.
	 *
	 * @param[in] state
	 *    the current aggregation state for producing the result element
	 * @param[in] timestamp
	 *    the timestamp for the result element
	 * @param[in] outdated
	 *    flag indicating if the result element should be marked as outdated
	 * @param[in] lock
	 *    a reference to the lock protecting the aggregation state
	 */
	void produceAggregate(const AggregateStatePtr& state, const Timestamp& timestamp,
		const bool outdated, const Lock& lock) {
		boost::ignore_unused(lock); // don't need it, just make sure that it exists
		auto tn = mFinalFunc(state);
		this->getOutputDataChannel().publish(tn, outdated);
	}


protected:

	/**
	 * TODO
	 */
  void notificationCallback() {
    Lock lock(mAggrMtx);
    PunctuationPtr punctuation = std::make_shared< Punctuation >( Punctuation::SlideExpired );
    this->getOutputPunctuationChannel().publish(punctuation);
  }

private:
    HashTable mAggregateTable;                  //< a hash table for storing the aggregation states for each group
    TimestampExtractorFunc mTimestampExtractor; //!< a pointer to the function for extracting the timestamp from the tuple
                                                //!< for each group at runtime
    mutable AggregationMutex mAggrMtx;           //!< a mutex for synchronizing access between the trigger notifier thread
                                                //!< and aggregation operator
    GroupByFunc mGroupByFunc;                   //!< a pointer to the function determining the key value for the group
    IterateFunc mIterateFunc;                   //!< a pointer to the iteration function called for each tuple
    FinalFunc mFinalFunc;                       //!< a  pointer to a function computing the final (or periodical) aggregates
		unsigned int mTriggerInterval;              //!< the interval (time in seconds, number of tuples) for publishing aggregates
    std::unique_ptr<TriggerNotifier> mNotifier; //!< the notifier object which triggers the computation of aggregates periodically
    Timestamp mLastTriggerTime;                 //!< the timestamp of the last aggregate publishing
    AggregationTriggerType mTriggerType;        //!< the type of trigger activating the publishing of an aggregate value
    unsigned int mCounter;                      //!< the number of tuples processed since the last aggregate publishing
};

} /* end namespace pfabric */



#endif /* GroupedAggregation_hpp_ */
