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
