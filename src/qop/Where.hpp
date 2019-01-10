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

#ifndef Where_hpp_
#define Where_hpp_

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

  /**
   * @brief A filter is a selection operator in a data stream.
   *
   * A filter is a selection operator in a data stream. It forwards all tuples
   * to its subscribers satisfying the given filter predicate.
   * Because a filter does not modify the tuple structure, the template is parameterized
   * only by one tuple type representing both input and output.
   *
   * @tparam StreamElement
   *    the data stream element type which shall be filtered
   */
  template<typename StreamElement>
  class Where : public UnaryTransform<StreamElement, StreamElement> {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

  public:

    /**
     * Typedef for a function pointer to a filter predicates.
     *
     * TODO make this a template argument for something more efficient like lambda type?
     */
    typedef std::function<bool(const StreamElement&, bool)> PredicateFunc;

    /**
     * Create a new filter operator evaluating the given predicate
     * on each incoming tuple.
     *
     * @param f function pointer to a filter predicate
     */
    Where(PredicateFunc f) : mFunc(f) {}

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, Where, processDataElement);

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, Where, processPunctuation);

    const std::string opName() const override { return std::string("Where"); }

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
      if(mFunc(data, outdated)) {
        this->getOutputDataChannel().publish(data, outdated);
      }
    }


    PredicateFunc mFunc; //< function pointer to the filter predicate
  };

} // namespace pfabric

#endif
