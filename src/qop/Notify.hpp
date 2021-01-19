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

#ifndef Notify_hpp_
#define Notify_hpp_

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

  /**
   * @brief Notify is an operator for triggering callbacks on each tuple of a data stream.
   *
   * Notify is an operator for triggering callbacks. It forwards all tuples
   * to its subscribers and invokes the callback for each tuple.
   * Because notify does not modify the tuple structure, the template is parameterized
   * only by one tuple type representing both input and output.
   *
   * @tparam StreamElement
   *    the input data stream element type
   */
  template<typename StreamElement>
  class Notify : public UnaryTransform<StreamElement, StreamElement> {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

  public:

    /**
     * Typedef for a function pointer to a callback function.
     */
    typedef std::function<void(const StreamElement&, bool)> CallbackFunc;

    typedef std::function<void(const PunctuationPtr&)> PunctuationCallbackFunc;
    /**
     * @brief Construct a new instance of the notify operator.
     *
     * Create a new notify operator invoking the given callback
     * on each incoming tuple.
     *
     * @param[in] f
     *      a function pointer or lambda function representing the callback
     *      that is invoked for each input tuple
     * @param[in] pf
     *      an optional function pointer or lambda function representing the callback
     *      that is invoked for each punctuation
     */
    Notify(CallbackFunc f, PunctuationCallbackFunc pf = nullptr) : mFunc(f), mPunctFunc(pf) {}

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, Notify, processDataElement);

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, Notify, processPunctuation);

    const std::string opName() const override { return std::string("Notify"); }

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
      if (mPunctFunc != nullptr)
        mPunctFunc(punctuation);
      this->getOutputPunctuationChannel().publish(punctuation);
    }

    /**
     * @brief This method is invoked when a stream element arrives from the publisher.
     *
     * It invokes the callback and forwards the incoming stream element.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement(const StreamElement& data, const bool outdated) {
      mFunc(data, outdated);
      this->getOutputDataChannel().publish(data, outdated);
    }


    CallbackFunc mFunc;                 //< function pointer to the callback
    PunctuationCallbackFunc mPunctFunc; //< function pointer to the punctuation callback
  };

} // namespace pfabric

#endif
