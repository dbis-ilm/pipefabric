/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#include <iostream>
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "Pattern.hpp"

using namespace std;

Pattern::Pattern(const std::vector<int>& vec, size_t f) : 
  seqData(make_shared<vector<int>>(vec)), start(0), len(vec.size()), freq(f) {
}

Pattern::Pattern(std::initializer_list<int> data, size_t f) {
  seqData = make_shared<vector<int>>(data);
  start = 0;
  len = seqData->size();
  freq = f;
}

Pattern::Pattern(const Pattern& seq, size_t f) : 
  seqData(seq.seqData), start(seq.start), len(seq.len), freq(f > 0 ? f : seq.freq) {
}

Pattern::Pattern(size_t f) : start(0), len(0), freq(f) {
}

Pattern::Pattern(shared_ptr<vector<int>> data, size_t s, size_t l, size_t f) : 
  seqData(data), start(s), len(l), freq(1) {
} 
  
int Pattern::operator[](size_t pos) const {
  return seqData->at(start + pos);
}

Pattern Pattern::slice(size_t s, size_t l) const {
  return Pattern(seqData, start + s, l == 0 ? length() - s : l);
}

Pattern Pattern::concat(int i) const {
  std::vector<int> seq;
  seq.insert(seq.begin(), seqData->begin() + start, seqData->begin() + start + length());
  seq.push_back(i);
  return Pattern(seq);
}

bool Pattern::contains(const Pattern& seq) const {
  if (len <= seq.len)
    return false;
  for (size_t i = 0; i <= len - seq.len; i++) {
    if (isEqualAt(i, seq))
      return true;
  }
  return false;
}

bool Pattern::isEqualN(const Pattern& seq, std::size_t maxLen) const {
  if (length() < maxLen || seq.length() < maxLen)
    return false;
  
  for (size_t i = 0; i < maxLen; i++) {
    if (seqData->at(i + start) != seq.seqData->at(i + seq.start))
      return false;
  }
  return true;
}

bool Pattern::isEqualAt(size_t pos, const Pattern& seq) const {
  if (len - pos < seq.len) 
    return false;

  for (size_t i = 0; i < seq.len; i++) {
    if ((*this)[i + pos] != seq[i])
      return false;
  }
  return true;
}

bool Pattern::hasPrefix(const Pattern& seq) const {
  if (seq.length() >= length())
    return false;
  return isEqualN(seq, seq.length());
}

bool Pattern::operator==(const Pattern& seq) const {
  if (len != seq.len) 
    return false;

  for (size_t i = 0; i < len; i++) {
    if (seqData->at(i + start) != seq.seqData->at(i + seq.start))
      return false;
  }
  return true;
}

int Pattern::findFirst(int elem) const {
  for (size_t i = 0; i < len; i++) {
    if ((*this)[i] == elem) 
      return i;
  }    
  return -1;
}

Pattern Pattern::findSuffix(const Pattern& pfx) const {
  auto first = findFirst(pfx[0]);
  if (first >= 0) {
    auto pmin = min(length(), pfx.len);
    for (size_t p = first + 1; p < pmin; p++) {
      if ((*this)[p] != pfx[p-first]) {
        return Pattern();
      }
    }
    return slice(first);
  }
  return Pattern();
}

list<Pattern> Pattern::findSubPatterns(size_t pfxLen) const {
  list<Pattern> result;
  for (size_t i = 0; i < len; i++) {
    if (i + pfxLen <= length()) {
      result.push_back(slice(i, pfxLen));
    }
  }
  return result;
}

string Pattern::toString() const {
  return fmt::format("{}", *this);
}

std::ostream &operator<<(std::ostream& os, const Pattern& seq) {
  if (seq.length() == 0)
    return os;
  os << seq[0];
  for (std::size_t i = 1; i < seq.length(); i++) 
    os << ',' << seq[i];
  return os;
}
