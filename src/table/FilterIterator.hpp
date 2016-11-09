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
 
#ifndef FilterIterator_hpp_
#define FilterIterator_hpp_

#include <iostream>
#include <iterator>

namespace pfabric {


template <typename Iter>
class FilterIterator :
  public std::iterator<
    std::bidirectional_iterator_tag,
    typename Iter::value_type::second_type
  > {
public:
  typedef typename Iter::value_type::second_type& reference;
  typename Iter::value_type::second_type* pointer;

  typedef std::function<bool(const typename Iter::value_type::second_type&)> Predicate;

  explicit FilterIterator() {}
  explicit FilterIterator(Iter j, Predicate p) : i(j), pred(p) {}
  FilterIterator& operator++() {
    while (! pred(i->second)) ++i;
    ++i;
    return *this;
  }
  FilterIterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }
  FilterIterator& operator--() { --i; return *this; }
  FilterIterator operator--(int) { auto tmp = *this; --(*this); return tmp; }
  bool operator==(FilterIterator j) const { return i == j.i; }
  bool operator!=(FilterIterator j) const { return !(*this == j); }
  typename Iter::value_type::second_type& operator*() { return i->second; }
  typename Iter::value_type::second_type* operator->() { return &i->second; }

protected:
  Iter i;
  Predicate pred;
};

template <typename Iter>
inline FilterIterator<Iter> makeFilterIterator(Iter j,
  typename FilterIterator<Iter>::Predicate p) { return FilterIterator<Iter>(j, p); }

}

#endif
