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

#ifndef Barrier_hpp_
#define Barrier_hpp_

#include <condition_variable>
#include <mutex>

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

  /**
   * @brief Barrier is an operator for delaying the forwarding of tuples
   *        of a stream based on a given predicate.
   *
   * The Barrier operator can be used to synchronize the processing of
   * a stream based on an external condition. For this purpose, the given
   * predicate function is evaluated and only if this predicate is satisfied
   * the tuple is forwarded to the subscribers. Otherwise, the tuple and
   * all subsequent tuples are blocked. The predicate is re-evaluated for the
   * tuple if the given condition variable is changed.
   *
   * @tparam StreamElement
   *    the data stream element type which shall be filtered
   */
  template<typename StreamElement>
  class Barrier : public UnaryTransform<StreamElement, StreamElement> {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

  public:

    /**
     * Typedef for a function pointer to a barrier predicates.
     */
    typedef std::function<bool(const StreamElement&)> PredicateFunc;

    /**
     * Create a new barrier operator evaluating the given predicate
     * on each incoming tuple.
     *
     * @param cVar a condition variable for signaling when the predicate
     *             shall be re-evaluated
     * @param mtx the mutex required to access the condition variable
     * @param f function pointer to a barrier predicate
     */
    Barrier(std::condition_variable& cVar, std::mutex& mtx, PredicateFunc f) :
      mCond(cVar), mMtx(mtx), mPred(f) {}

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, Barrier, processDataElement);

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, Barrier, processPunctuation);

    const std::string opName() const override { return std::string("Barrier"); }

  private:

    /**
     * @brief This method is invoked when a punctuation arrives.
     *
     * It simply forwards the @c punctuation to the subscribers.
     *
     * @param[in] punctuation
     *    the incoming punctuation tuple
     */
    void processPunctuation(const PunctuationPtr& punctuation) {
      this->getOutputPunctuationChannel().publish(punctuation);
    }

    /**
     * @brief This method is invoked when a stream element arrives from the publisher.
     *
     * It forwards the incoming stream element if it satisfies the filter predicate.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement(const StreamElement& data, const bool outdated) {
      std::unique_lock<std::mutex> lock(mMtx);
      while (!mPred(data)) {
        mCond.wait(lock);
      }
      this->getOutputDataChannel().publish(data, outdated);
    }


    std::condition_variable& mCond;  //< condition variable for checking the predicate
    std::mutex& mMtx;                //< mutex for accessing the condition variable
    PredicateFunc mPred;             //< function pointer to the barrier predicate
  };

} // namespace pfabric

#endif
