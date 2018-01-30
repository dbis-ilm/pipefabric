/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef SHJoin_hpp_
#define SHJoin_hpp_

#include <boost/core/ignore_unused.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <unordered_map>

#include "qop/BinaryTransform.hpp"
#include "ElementJoinTraits.hpp"
#include "DefaultElementJoin.hpp"

namespace pfabric {

  /**
   * \brief An operator implementing a symmetric hash join for computing equi-joins.
   *
   * The symmetric hash join operator joins two input streams on a given join predicate. Because it is based on
   * a hash join, the key columns used for deriving the hash keys have to be specified, too. Note, that the predecessor
   * operators of a symmetric hash join should be window operators to ensure that tuples are invalidated after some time.
   * Otherwise, tuples are never removed from the hash tables.
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
  class SHJoin : public BinaryTransform<LeftInputStreamElement, RightInputStreamElement,
  typename ElementJoinTraits< ElementJoinImpl >::ResultElement>{
    private:
      PFABRIC_BINARY_TRANSFORM_TYPEDEFS(LeftInputStreamElement, RightInputStreamElement, typename ElementJoinTraits< ElementJoinImpl >::ResultElement);

    public:
      /**
       * Typedef for the key extractor functions.
       */
      typedef std::function<KeyType(const LeftInputStreamElement&)> LKeyExtractorFunc;
      typedef std::function<KeyType(const RightInputStreamElement&)> RKeyExtractorFunc;

      /**
       * Typedef for the pointer to a function implementing the join predicate.
       */
      typedef std::function< bool(const LeftInputStreamElement&, const RightInputStreamElement&) > JoinPredicateFunc;

    private:
      /**
       * The type definition for our hash tables: we use the native Boost implementation.
       * Because, we allow that stream elements have the same key, we need a multimap here.
       */
      typedef std::unordered_multimap< KeyType, LeftInputStreamElement > LHashTable;
      typedef std::unordered_multimap< KeyType, RightInputStreamElement > RHashTable;

      /// the join algorithm to be used for concatenating the input elements
      typedef ElementJoinTraits< ElementJoinImpl > ElementJoin;

      /// a mutex for protecting join processing from concurrent sources
      typedef boost::mutex JoinMutex;

      /// a scoped lock for the mutex
      typedef boost::lock_guard< JoinMutex > Lock;

    public:

      /// the join result for two input elements
      typedef typename ElementJoin::ResultElement ResultElement;


      /**
       * Constructs a new hash join operator subscribing to two source operators producing the
       * left and right hand-side input data streams. The operator implements a symmetric hash join.
       *
       * \param lhs_hash function pointer to the hash function for tuples of the lhs stream
       * \param rhs_hash function pointer to the hash function for tuples of the rhs stream
       * \param join_pred function pointer to a join predicate
       */
      SHJoin( LKeyExtractorFunc lKeyFunc, RKeyExtractorFunc rKeyFunc, JoinPredicateFunc joinPred) :
      mJoinPredicate(joinPred), mLKeyExtractor(lKeyFunc), mRKeyExtractor(rKeyFunc) {
      }

      /**
       * @brief Bind the callback for the left handside data channel.
       */
      BIND_INPUT_CHANNEL_DEFAULT( LeftInputChannel, SHJoin, processLeftDataElement );

      /**
       * @brief Bind the callback for the data channel.
       */
      BIND_INPUT_CHANNEL_DEFAULT( RightInputChannel, SHJoin, processRightDataElement );

      /**
       * @brief Bind the callback for the punctuation channel.
       */
      BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, SHJoin, processPunctuation );


    private:

      ////////////   channel callbacks   ////////////

      /**
       * @brief This method is invoked when a data stream element arrives from the left input channel.
       *
       * It inserts the element into the corresponding hash table and tries to join it with
       * elements from the other hash table.
       *
       * @param[in] left
       *    the incoming stream element from the left input channel
       * @param[in] outdated
       *    flag indicating whether the tuple is new or invalidated now
       */
      void processLeftDataElement( const LeftInputStreamElement& left, const bool outdated ) {
        Lock lock( mMtx );

        // 1. insert the tuple in the corresponding hash table or remove it if outdated
        auto keyval = mLKeyExtractor( left );
        updateHashTable( mLTable, keyval, left, outdated, lock );

        // 2. find join partners in the other hash table
        auto rightEqualElements = mRTable.equal_range(keyval);
        for (auto rightElementEntry = rightEqualElements.first; rightElementEntry != rightEqualElements.second; rightElementEntry++) {
          // 3. join both tuples
          joinTuples( left, rightElementEntry->second, outdated);
        }
      }

      /**
       * @brief This method is invoked when a data stream element arrives from the right input channel.
       *
       * It inserts the element into the corresponding hash table and tries to join it with
       * elements from the other hash table.
       *
       * @param[in] right
       *    the incoming stream element from the right input channel
       * @param[in] outdated
       *    flag indicating whether the tuple is new or invalidated now
       */
      void processRightDataElement( const RightInputStreamElement& right, const bool outdated ) {
        Lock lock( mMtx );

        // 1. insert the tuple in the corresponding hash table or remove it if outdated
        auto keyval = mRKeyExtractor( right );
        updateHashTable( mRTable, keyval, right, outdated, lock );

        // 2. find join partners in the other hash table
        auto leftEqualElements = mLTable.equal_range( keyval );
        for (auto leftElementEntry = leftEqualElements.first; leftElementEntry != leftEqualElements.second; leftElementEntry++) {
          // 3. join both tuples
          joinTuples( leftElementEntry->second, right, outdated);
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
      void processPunctuation( const PunctuationPtr& punctuation ) {
        this->getOutputPunctuationChannel().publish( punctuation );
      }


      ////////////   helper methods   ////////////

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
       * @param[in] lock
       *    reference to the lock protecting the hash table
       */
      template<
      typename HashTable,
      typename StreamElement
      >
      static void updateHashTable( HashTable& hashTable, const KeyType& key,
                                  const StreamElement& newElement, const bool outdated, const Lock& lock ) {
        boost::ignore_unused( lock );

        if( !outdated ) {
          hashTable.insert( { key, newElement });
        }
        else {
          auto equalElements = hashTable.equal_range( key );
          auto equalElementEntry = equalElements.first;
          while (equalElementEntry != equalElements.second) {
            const auto& equalElement = equalElementEntry->second;

            /*
             * TODO Question: Does one outdated tuple remove all equal ones?
             *                Equality check needed here? -> Doesn't the hash imply this?
             */
            if( elementsEqual( newElement, equalElement ) ) {
              equalElementEntry = hashTable.erase( equalElementEntry );
            }
            else {
              equalElementEntry++;
            }
          }
        }
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
      void joinTuples( const LeftInputStreamElement& left, const RightInputStreamElement& right,
                      const bool outdated) {
        if( mJoinPredicate( left, right ) ) {
          // 4. if satisfied then publish the result
          ResultElement joinedTuple = ElementJoin::joinElements( left, right );
          this->getOutputDataChannel().publish( joinedTuple, outdated );
        }
      }

      LHashTable mLTable;               //< hash table for the lhs stream
      RHashTable mRTable;               //< hash table for the rhs stream
      JoinPredicateFunc mJoinPredicate; //< a pointer to the function implementing the join predicate
      LKeyExtractorFunc mLKeyExtractor;         //< hash function for the lhs stream
      RKeyExtractorFunc mRKeyExtractor;         //< hash function for the rhs stream
      mutable JoinMutex mMtx;
    };

} /* end namespace pfabric */


#endif
