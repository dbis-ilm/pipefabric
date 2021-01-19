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

#ifndef StreamGenerator_hpp_
#define StreamGenerator_hpp_

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

  /**
   * @brief A StreamGenerator operator creates a stream of tuples
   * produced by a generator function.
   *
   * StreamGenerator is an operator for generating a stream of a given number 
   * of tuples. For this purpose, a generator function can be specified which
   * produces one tuple per call.
   *
   * @tparam StreamElement
   *    the data stream element type which shall be retrieve from the table
   */
  template<typename StreamElement>
  class StreamGenerator : public DataSource<StreamElement> {
  public:
    PFABRIC_SOURCE_TYPEDEFS(StreamElement);

    //< typedef for the generator function - the parameter refers to the tuple number
    typedef std::function<StreamElement(unsigned long)> Generator;

    StreamGenerator(Generator gen, unsigned long numTuples) : mGenerator(gen), mNumTuples(numTuples) {}

    unsigned long start() {
      for (unsigned long i = 0; i < mNumTuples; i++) {
        auto tup = mGenerator(i);
        this->getOutputDataChannel().publish(tup, false);
      }

      // publish punctuation
      this->getOutputPunctuationChannel().publish(PunctuationPtr(new Punctuation(Punctuation::EndOfStream)));
      return mNumTuples;
    }

  private:
    Generator mGenerator;      //< the generator function which produces a tuple for each call
    unsigned long mNumTuples;  //< the number of tuples to be produced
  };

}

#endif

