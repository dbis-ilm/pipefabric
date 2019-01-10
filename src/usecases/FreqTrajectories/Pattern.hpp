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

#ifndef Pattern_hpp_
#define Pattern_hpp_

#include <memory>
#include <vector>
#include <list>
#include <initializer_list>

class Pattern {
protected:
  Pattern(std::shared_ptr<std::vector<int>> data, std::size_t s, std::size_t l, std::size_t f = 1);

public:
  Pattern(const std::vector<int>& vec, std::size_t f = 1);
  Pattern(std::initializer_list<int> data, std::size_t f = 1);
  Pattern(const Pattern& seq, std::size_t f = 0);
  Pattern(std::size_t f = 1);

  bool contains(const Pattern& seq) const;

  bool isEmpty() const { return len == 0 || !seqData; }
  std::size_t length() const { return len; }

  bool operator==(const Pattern& seq) const;
  bool isEqualAt(std::size_t pos, const Pattern& seq) const; 
  bool isEqualN(const Pattern& seq, std::size_t maxLen) const;

  int operator[](std::size_t pos) const;
  Pattern slice(std::size_t s, std::size_t l = 0) const;
  Pattern concat(int i) const;

  Pattern findSuffix(const Pattern& seq) const;

  bool hasPrefix(const Pattern& seq) const;

  std::list<Pattern> findSubPatterns(std::size_t len) const;

  std::string toString() const;

  void incrFrequency(std::size_t incr = 1) { freq += incr; }
  std::size_t frequency() const { return freq; }

friend std::ostream &operator<<(std::ostream& os, const Pattern& seq);

protected:
  int findFirst(int elem) const;

  std::shared_ptr<std::vector<int>> seqData;
  std::size_t start, len;
  std::size_t freq;
};

#endif
