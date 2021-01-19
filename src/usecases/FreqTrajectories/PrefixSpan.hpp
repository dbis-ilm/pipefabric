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

#ifndef PrefixSpan_hpp_
#define PrefixSpan_hpp_

#include <list>
#include <string>

#include "Pattern.hpp"


class PrefixSpan {
public:
  typedef std::list<Pattern> PatternList;

  PrefixSpan(std::size_t minSupp) : minSupport(minSupp) {}

  PatternList mineFreqPatterns(const PatternList& dataSet);

  void mineFreqPatterns(const PatternList& dataSet, std::size_t pfxLen, PatternList& result);

  PatternList findFreqPrefixes(const PatternList& dataSet, std::size_t pfxLen);

  PatternList projectedPatternDB(const PatternList& dataSet, const Pattern& pfx);

  PatternList growPattern(const PatternList& dataSet, const Pattern& pattern, std::size_t pfxLen);

  static PatternList suppressSubPatterns(const PatternList& dataSet);

private:
  std::size_t minSupport;
};

#endif
