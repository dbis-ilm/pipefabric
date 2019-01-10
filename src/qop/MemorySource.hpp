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

#ifndef MemorySource_hpp_
#define MemorySource_hpp_

#include <fstream>
#include <iostream>
#include <memory>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/DataSource.hpp"
#include "qop/Map.hpp"
#include "qop/Notify.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "qop/TupleExtractor.hpp"

namespace pfabric {

/**
 * @brief TextFileSource is a source operator for reading a text file line by
 *        line and producing a stream of tuples.
 *
 * A FileSource is an operator producing a stream of tuples which are extracted
 * from a file.
 * We assume a simple text file where a record is represented by a separate
 * line. The operator produces a stream of @c TStringPtr elements.
 */
template <typename StreamElement>
class MemorySource : public DataSource<StreamElement> {
 public:
  PFABRIC_SOURCE_TYPEDEFS(StreamElement)

  /**
   * Creates a new FileSource implementation object for reading data from a file
   * and producing tuples.
   *
   * @param fname the name of the file we read the data from
   */
  MemorySource(const std::string& fname, char delim = ',',
               unsigned long limit = 0)  {
    if (limit > 0) data.reserve(limit);
    fileSource = std::make_shared<TextFileSource>(fname, limit);
    extractor = std::make_shared<TupleExtractor<StreamElement>>(delim);
    CREATE_LINK(fileSource, extractor);
    notify =
        std::make_shared<Notify<StreamElement>>([&](auto tp, bool outdated) {
          data.push_back(tp);
        });
    CREATE_LINK(extractor, notify);
  }

  /**
   * Deallocates all resources.
   */
  ~MemorySource() {}

  unsigned long prepare() { return fileSource->start(); }

  /**
   * Performs the actual processing by reading the file, parsing the input
   * tuples and send
   * the to the subscribers. This method has to be invoked explicitly.
   *
   * @return the number of tuples produced
   */
  unsigned long start() {
    for (auto tp : data) {
      this->getOutputDataChannel().publish(tp, false);
    }
    this->getOutputPunctuationChannel().publish(
        std::make_shared<Punctuation>(Punctuation::EndOfStream));
    return data.size();
  }

  const std::string opName() const override {
    return std::string("MemorySource");
  }

 private:
  std::shared_ptr<TextFileSource> fileSource;
  std::shared_ptr<TupleExtractor<StreamElement>> extractor;
  std::shared_ptr<Notify<StreamElement>> notify;
  std::vector<StreamElement> data;
};
}

#endif
