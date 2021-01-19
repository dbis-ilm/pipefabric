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

#ifndef JsonExtractor_hpp_
#define JsonExtractor_hpp_

#include <vector>
#include <string>

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"
#include "json/json.hpp"

namespace pfabric {

  /**
   * \brief An operator for extracting a tuple from a JSON string.
   *
   * A JsonExtractor operator produces tuples from a JSON string by using
   * the list of keys specified as parameter and assign the corresponding values
   * to the fields of the output tuple those structure is defined by the
   * OutputStreamElement type.
   *
   * @tparam InputStreamElement
   *    the data stream element type consumed by the extractor
   * @tparam OutputStreamElement
   *    the data stream element type produced by the extractor
   */
  template<
  typename OutputStreamElement
  >
  class JsonExtractor :
  public UnaryTransform< TStringPtr, OutputStreamElement > {
  private:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(TStringPtr, OutputStreamElement);

  public:

    /**
     * Create a new projection operator for evaluating the projection function
     * on each incoming tuple.
     *
     * @param keys a list of keys for which values extracted from the JSON string
     *             and used to construct the tuple
     */
    JsonExtractor(const std::vector<std::string>& keys) : mKeys(keys) {
        BOOST_ASSERT_MSG(mKeys.size() == OutputDataElementTraits::NUM_ATTRIBUTES,
          "insufficient number of keys in JsonExtractor");
        mData.resize(OutputDataElementTraits::NUM_ATTRIBUTES);
      }

    ~JsonExtractor() {}

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, JsonExtractor, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, JsonExtractor, processPunctuation );


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
     * It performs the actual extraction from the JSON string and
     * forwards the constructed tuple to its subscribers.
     *
     * @param[in] line
     *    the incoming stream element (a TStringPtr)
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement( const TStringPtr& line, const bool outdated ) {
      using json = nlohmann::json;

      auto j = json::parse(line->getAttribute<0>());
      for (auto i = 0u; i < mKeys.size(); i++) {
        if (j.find(mKeys[i]) == j.end()) {
          mData[i] = "";
          continue;
        }
        auto val = j[mKeys[i]];
        // TODO: this is a very inefficient way: why to convert
        // the value to a string which is parsed in create again
        if (val.is_number_integer()) {
          mData[i] = std::to_string((int)val);
        }
        else if (val.is_number_float()) {
          mData[i] = std::to_string((float)val);
        }
        else if (val.is_string()) {
          mData[i] = val;
        }
      }
      auto res = OutputDataElementTraits::create(mData);
      this->getOutputDataChannel().publish( res, outdated );
    }


    std::vector<std::string> mKeys;   //< a list of keys used to extract JSON data
    StringTuple mData;                //< a tuple of string values used for intermediate
                                      //< data but created only once
  };

} // namespace pfabric

#endif
