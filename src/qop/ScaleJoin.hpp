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

#ifndef ScaleJoin_hpp_
#define ScaleJoin_hpp_

#include <unordered_map>

#include "qop/BinaryTransform.hpp"
#include "ElementJoinTraits.hpp"
#include "DefaultElementJoin.hpp"

namespace pfabric {

  /**
   * \brief An operator implementing a ScaleJoin.
   * Origin idea & paper: "ScaleJoin: a Deterministic, Disjoint-Parallel and Skew-Resilient Stream Join" (2016)
   *
   * The ScaleJoin operator joins two input streams on a given join predicate. Each ScaleJoin instance gets all
   * tuples from both streams, but stores only the tuples belonging to it's ID. Therefore each incoming tuple is
   * only stored once in one of the ScaleJoin instances, reducing the overall memory usage.
   *
   * @tparam LeftInputStreamElement
   *    the data stream element type from the left source
   * @tparam RightInputStreamElement
   *    the data stream element type from the right source
   * @tparam ElementJoinImpl
   *    the actual join algorithm to be used for joining two input elements
   */
  template<
  typename LeftInputStreamElement,
  typename RightInputStreamElement,
  typename KeyType = DefaultKeyType,
  typename ElementJoinImpl = DefaultElementJoin< LeftInputStreamElement, RightInputStreamElement >
  >
  class ScaleJoin : public BinaryTransform<LeftInputStreamElement, RightInputStreamElement,
  typename ElementJoinTraits< ElementJoinImpl >::ResultElement>{
    private:
      PFABRIC_BINARY_TRANSFORM_TYPEDEFS(LeftInputStreamElement, RightInputStreamElement, typename ElementJoinTraits< ElementJoinImpl >::ResultElement);

    public:
      //typedef for the key extractor functions
      typedef std::function<KeyType(const LeftInputStreamElement&)> LKeyExtractorFunc;
      typedef std::function<KeyType(const RightInputStreamElement&)> RKeyExtractorFunc;

      //typedef for the pointer to a function implementing the join predicate
      typedef std::function< bool(const LeftInputStreamElement&, const RightInputStreamElement&) > JoinPredicateFunc;


    private:
      //the type definition for the hash tables - because of allowing stream elements to have the same key, a multimap is necessary
      typedef std::unordered_multimap< KeyType, LeftInputStreamElement > LHashTable;
      typedef std::unordered_multimap< KeyType, RightInputStreamElement > RHashTable;

      //the join algorithm to be used for concatenating the input elements
      typedef ElementJoinTraits< ElementJoinImpl > ElementJoin;


    public:
      //the join result for two input elements
      typedef typename ElementJoin::ResultElement ResultElement;

      /**
       * Constructs a new ScaleJoin operator subscribing to two source operators producing the
       * left and right hand-side input data streams.
       *
       * \param lhs_hash function pointer to the hash function for tuples of the lhs stream
       * \param rhs_hash function pointer to the hash function for tuples of the rhs stream
       * \param join_pred function pointer to a join predicate
       * \param id unique id for the thread, used for deciding which tuples to store
       * \param numThreads amount of ScaleJoin instances (threads)
       */
      ScaleJoin(LKeyExtractorFunc lKeyFunc, RKeyExtractorFunc rKeyFunc, JoinPredicateFunc joinPred, const int id, const int numThreads) :
      mJoinPredicate(joinPred), mLKeyExtractor(lKeyFunc), mRKeyExtractor(rKeyFunc), mID(id), mThreadnum(numThreads) {
        //initialize counters to zero
        mLCntr = mRCntr = mLOCntr = mROCntr = 0;
      }

      //bind the callback for the left handside data channel
      BIND_INPUT_CHANNEL_DEFAULT(LeftInputChannel, ScaleJoin, processLeftDataElement);

      //bind the callback for the right data channel
      BIND_INPUT_CHANNEL_DEFAULT(RightInputChannel, ScaleJoin, processRightDataElement);

      //bind the callback for the punctuation channel
      BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, ScaleJoin, processPunctuation);


    private:
      /**
       * @brief This method is invoked when a data stream element arrives from the left input channel.
       *
       * The element is inserted into the corresponding hash table if (according to the ID) this ScaleJoin
       * instance is responsible for storing the tuple. However, it always tries to join it with elements
       * from the other hash table.
       *
       * @param[in] left
       *    the incoming stream element from the left input channel
       * @param[in] outdated
       *    flag indicating whether the tuple is new or invalidated now
       */
      void processLeftDataElement(const LeftInputStreamElement& left, const bool outdated) {
        //extract the key from the tuple
        auto keyval = mLKeyExtractor(left);

        //if tuple is outdated
        if(outdated) {
          //if left outdated counter matches own ID, this instance is responsible for storing the tuple,
          //therefore it should have seen it before, so it can be removed from the hash table
          if(mLOCntr==mID) {
            updateHashTable(mLTable, keyval, left, outdated);
          }
          //increase counter for left outdated tuples and verify that the counter is not higher than the
          //maximum number of ScaleJoin instances (round robin principle)
          mLOCntr++;
          mLOCntr%=mThreadnum;

        //if tuple is not outdated
        } else {
          //if left tuple counter matches own ID, this instance is responsible for storing the tuple,
          //so it is inserted into the left hash table
          if(mLCntr==mID) {
            updateHashTable(mLTable, keyval, left, outdated);
          }
          //increase counter for left tuples and verify that the counter is not higher than the
          //maximum number of ScaleJoin instances (round robin principle)
          mLCntr++;
          mLCntr%=mThreadnum;
        }

        //try to find match in the right hash table
        auto rightEqualElements = mRTable.equal_range(keyval);

        //for all matching tuples, join them
        for (auto rightElementEntry = rightEqualElements.first; rightElementEntry != rightEqualElements.second; rightElementEntry++) {
          joinTuples(left, rightElementEntry->second, outdated);
        }
      }

      /**
       * @brief This method is invoked when a data stream element arrives from the right input channel.
       *
       * The element is inserted into the corresponding hash table if (according to the ID) this ScaleJoin
       * instance is responsible for storing the tuple. However, it always tries to join it with elements
       * from the other hash table.
       *
       * @param[in] right
       *    the incoming stream element from the right input channel
       * @param[in] outdated
       *    flag indicating whether the tuple is new or invalidated now
       */
      void processRightDataElement(const RightInputStreamElement& right, const bool outdated) {
        //extract the key from the tuple
        auto keyval = mRKeyExtractor(right);

        //if tuple is outdated
        if(outdated) {
          //if right outdated counter matches own ID, this instance is responsible for storing the tuple,
          //therefore it should have seen it before, so it can be removed from the hash table
          if(mROCntr==mID) {
            updateHashTable(mRTable, keyval, right, outdated);
          }
          //increase counter for right outdated tuples and verify that the counter is not higher than the
          //maximum number of ScaleJoin instances (round robin principle)
          mROCntr++;
          mROCntr%=mThreadnum;

        //if tuple is not outdated
        } else {
          //if right tuple counter matches own ID, this instance is responsible for storing the tuple,
          //so it is inserted into the right hash table
          if(mRCntr==mID) {
            updateHashTable(mRTable, keyval, right, outdated);
          }
          //increase counter for right tuples and verify that the counter is not higher than the
          //maximum number of ScaleJoin instances (round robin principle)
          mRCntr++;
          mRCntr%=mThreadnum;
        }

        //try to find match in the left hash table
        auto leftEqualElements = mLTable.equal_range(keyval);

        //for all matching tuples, join them
        for (auto leftElementEntry = leftEqualElements.first; leftElementEntry != leftEqualElements.second; leftElementEntry++) {
          joinTuples(leftElementEntry->second, right, outdated);
        }
      }

      /**
       * @brief This method is invoked when a punctuation arrives.
       *
       * It simply forwards the punctuation to the subscribers.
       *
       * @param[in] punctuation
       *    the incoming punctuation tuple
       */
      void processPunctuation(const PunctuationPtr& punctuation) {
        this->getOutputPunctuationChannel().publish(punctuation);
      }

      /**
       * @brief Update a hash table for a new input element.
       *
       * @tparam HashTable
       *    the type of the hash table to be updated
       * @param[in] hashTable
       *    reference to the hash table to be updated
       * @param[in] key
       *    the hash key for the new element
       * @param[in] newElement
       *    the new element
       * @param[in] outdated
       *    flag indicating whether the tuple is new or invalidated now
       */
      template<typename HashTable, typename StreamElement>
      static void updateHashTable(HashTable& hashTable, const key_t& key,
                                  const StreamElement& newElement, const bool outdated) {

        //if not outdated, just insert it into the hash table
        if(!outdated) {
          hashTable.insert({key, newElement});

        //if outdated
        } else {
          //get all tuples with the same key
          auto equalElements = hashTable.equal_range(key);
          //pointer to first element
          auto equalElementEntry = equalElements.first;

          //run through all tuples, remove matching tuples from hash table
          while (equalElementEntry != equalElements.second) {
            const auto& equalElement = equalElementEntry->second;

            if(elementsEqual(newElement, equalElement)) {
              equalElementEntry = hashTable.erase(equalElementEntry);
            } else {
              equalElementEntry++;
            }
          }//end while
        }//end outdated
      }

      /**
       * @brief Join two tuples and publish the result.
       *
       * This method joins two input tuples and produces a result if the join predicate matches.
       *
       * @param[in] left
       *    the tuple from the left handside of the join
       * @param[in] right
       *    the tuple from the right handside of the join
       * @param[in] outdated
       *    flag indicating whether the tuple is new or invalidated now
       */
      void joinTuples(const LeftInputStreamElement& left, const RightInputStreamElement& right,
                      const bool outdated) {

        //if join predicate matches
        if(mJoinPredicate(left, right)) {
          //execute join
          ResultElement joinedTuple = ElementJoin::joinElements(left, right);
          //publish to following operator
          this->getOutputDataChannel().publish(joinedTuple, outdated);
        }
      }

      LHashTable mLTable;                     //hash table for the left stream
      RHashTable mRTable;                     //hash table for the right stream
      JoinPredicateFunc mJoinPredicate;       //pointer to the function implementing the join predicate
      LKeyExtractorFunc mLKeyExtractor;       //function for extracting the key of the left stream
      RKeyExtractorFunc mRKeyExtractor;       //function for extracting the key of the right stream
      const int mID;                          //unique ID of this ScaleJoin instance
      const int mThreadnum;                   //number of all ScaleJoin instances
      short mLCntr, mRCntr, mLOCntr, mROCntr; //counters for left and right stream tuples (+outdated)
    };
}

#endif
