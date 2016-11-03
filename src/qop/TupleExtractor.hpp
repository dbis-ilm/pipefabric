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
   * @brief An operator for extracting tuple fields from a single string.
   *
   * A TupleExtractor operator produces structured tuples of the given template
   * type parameter type from a tuple consisting only of a single string. The
   * separator character can be specified in the contstructor.
   *
   * @tparam OutputStreamElement
   *    the data stream element type produced by the extractor
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
     * Create a new TupleExtractor operator for transforming a string into
     * a structured tuple with separate fields based on the given field
     * separator.
     *
     * @param separator the character for separating fields in the input string
     */
    TupleExtractor(char separator = ',') :
      ifs(separator),
      data(new StringRef[OutputDataElementTraits::NUM_ATTRIBUTES]),
      nulls(new bool[OutputDataElementTraits::NUM_ATTRIBUTES]) {}

    /**
     * Desctructor for deallocating resources.
     */
    ~TupleExtractor() {
      delete [] data;
      delete [] nulls;
    }
    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, TupleExtractor, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, TupleExtractor, processPunctuation );

    const std::string opName() const override { return std::string("TupleExtractor"); }

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
     * This method is invoked when a data stream element arrives. It splits the
     * input string based on the separator character and tries to parse the
     * values according to the template parameter type. The resulting tuple
     * is then forwarded  to the subscribers.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement( const TStringPtr& line, const bool outdated ) {
      auto s = line->getAttribute<0>().begin();
      unsigned i = 0;
      while (*s) {
        char* item = (char *)s;
        while (*s && *s != ifs) s++;
        if ((s - item) == 0) {
          data[i].setValues(nullptr, 0);
          nulls[i] = true;
        }
        else {
          data[i].setValues(item, s - item);
          nulls[i] = false;
        }
        s++; i++;
        if (i == OutputDataElementTraits::NUM_ATTRIBUTES)
          break;
      }
      while (i < OutputDataElementTraits::NUM_ATTRIBUTES) {
        data[i].setValues(nullptr, 0);
        nulls[i++] = true;
      }
      auto res = OutputDataElementTraits::create(data);
      // because the tuple parser doesn't handle null values we
      // have to set the null flag manually
      for (auto n = 0; n < OutputDataElementTraits::NUM_ATTRIBUTES; n++) {
        if (nulls[n]) res->setNull(n);
      }
      this->getOutputDataChannel().publish( res, outdated );
    }


    char ifs;         //< the field separator
    StringRef *data;  //< a field of strings used to parse the values which is
                      //< reused for all tuples
    bool *nulls;
  };

} // namespace pfabric

#endif
