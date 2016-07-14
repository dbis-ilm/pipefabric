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

#ifndef TupleExtractor_hpp_
#define TupleExtractor_hpp_

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

  /**
   * \brief An operator implementing the relational projection.
   *
   * A projection operator produces tuples according to a given projection function.
   *
   * @tparam InputStreamElement
   *    the data stream element type consumed by the projection
   * @tparam OutputStreamElement
   *    the data stream element type produced by the projection
   */
  template<
  typename OutputStreamElement
  >
  class TupleExtractor :
  public UnaryTransform< TStringPtr, OutputStreamElement > {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(TStringPtr, OutputStreamElement);

  public:

    /**
     * Create a new projection operator for evaluating the projection function
     * on each incoming tuple.
     *
     * \param pfun function pointer to a projection function
     */
    TupleExtractor(char separator = ',') : ifs(separator),
    data(new StringRef[OutputDataElementTraits::NUM_ATTRIBUTES]) {}

    ~TupleExtractor() {
      delete [] data;
    }
    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, TupleExtractor, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, TupleExtractor, processPunctuation );


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
     * It applies the projection function and forwards the projected element to its subscribers.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement( const TStringPtr& line, const bool outdated ) {
      auto s = line->getAttribute<0>().begin();
      int i = 0;
      while (*s) {
        char* item = (char *)s;
        while (*s && *s != ifs) s++;
        BOOST_ASSERT(i < OutputDataElementTraits::NUM_ATTRIBUTES);
        data[i++].setValues(item, s - item);
        if (*s && !*(++s)) {
          data[i++].setValues(nullptr, 0);
        }
      }
      auto res = OutputDataElementTraits::create(data);
      this->getOutputDataChannel().publish( res, outdated );
    }


    char ifs;  //< function pointer to the projection function
    StringRef *data;
  };

} // namespace pfabric

#endif
