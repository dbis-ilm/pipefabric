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

#ifndef TupleDeserializer_hpp_
#define TupleDeserializer_hpp_

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/ZMQSource.hpp"

namespace pfabric {

  /**
   * @brief An operator for deserializing tuples from a byte array.
   *
   * A TupleDeserializer takes in stream element that contains a byte array
   * storing a serialized tuple and extracts the original tuple. This is mainly
   * used for sending tuples via ZMQSink/ZMQSource or writing them to external
   * storage.
   *
   * @tparam OutputStreamElement
   *    the data stream element type produced by the deserializer
   */
  template<
  typename OutputStreamElement
  >
  class TupleDeserializer :
  public UnaryTransform< TBufPtr, OutputStreamElement > {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(TBufPtr, OutputStreamElement);

  public:

    /**
     * Create a new TupleDeserializer operator.
     */
    TupleDeserializer() {}

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, TupleDeserializer, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, TupleDeserializer, processPunctuation );


  private:

    /**
     * @brief This method is invoked when a punctuation arrives.
     *
     * It simply forwards the punctuation to the subscribers.
     *
     * @param[in] punctuation
     *    the incoming punctuation tuple
     */
    void processPunctuation( const PunctuationPtr& punctuation ) {
      this->getOutputPunctuationChannel().publish(punctuation);
    }

    /**
     * This method is invoked when a data stream element arrives.
     *
     * It deserializes the tuple from the byte array @buf and forwards 
     * the result to the subscribers.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement( const TBufPtr& buf, const bool outdated ) {
      StreamType res = buf->getAttribute<0>();
      auto tp = OutputDataElementTraits::create(res);
      this->getOutputDataChannel().publish( tp, outdated );
    }
  };

} // namespace pfabric

#endif
