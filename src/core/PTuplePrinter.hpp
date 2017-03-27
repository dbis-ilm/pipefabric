/*
 * Copyright (c) 2014-17 The PipeFabric team,
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

#ifndef PTuplePrinter_hpp_
#define PTuplePrinter_hpp_

#include <ostream>
#include <core/PTuple.hpp>

namespace pfabric {

/**
 * Forward declarations
 */
namespace nvm {
template<class Tuple> class PTuple;
}
template<std::size_t ID, class Tuple>
auto get(const nvm::PTuple<Tuple>& ptp) -> typename Tuple::template getAttributeType<ID>::type;

namespace nvm {

namespace detail {

template<class Tuple, std::size_t CurrentIndex>
struct PTuplePrinter;

template<class Tuple, std::size_t CurrentIndex>
struct PTuplePrinter {
  static void print(std::ostream& os, const nvm::PTuple<Tuple>& ptp) {
    PTuplePrinter<Tuple, CurrentIndex - 1>::print(os, ptp);
    os << "," << pfabric::get<CurrentIndex - 1>(ptp);
  }
};

template<class Tuple>
struct PTuplePrinter<Tuple, 1> {
  static void print(std::ostream& os, const nvm::PTuple<Tuple>& ptp) {
    os << pfabric::get<0>(ptp);
  }
};

template<class Tuple>
struct PTuplePrinter<Tuple, 0> {
  static void print(std::ostream& os, const nvm::PTuple<Tuple>& ptp) {
  }
};

} /* end namespace detail */

template<class Tuple>
void print(std::ostream& os, const nvm::PTuple<Tuple>& ptp) {
  detail::PTuplePrinter<Tuple, ptp.NUM_ATTRIBUTES>::print(os, ptp);
}

} /* end namespace nvm */
} /* end namespace pfabric */

#endif /* PTuplePrinter_hpp_ */
