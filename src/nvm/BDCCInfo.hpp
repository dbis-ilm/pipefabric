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

#ifndef BDCCInfo_hpp_
#define BDCCInfo_hpp_

#include <map>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include "nvm/PTableInfo.hpp"
#include <numeric>

#include "nvml/include/libpmemobj++/allocator.hpp"
#include "nvml/include/libpmemobj++/p.hpp"

namespace pfabric {
namespace nvm {

using nvml::obj::allocator;
using nvml::obj::p;

/**************************************************************************//**
 * \brief Info structure about the BDCC meta data.
 *
 * It is used in persistent tables to store the BDCC meta data and statistics.
 *****************************************************************************/
class BDCCInfo {
  using DimensionUses = std::vector<std::tuple<uint16_t, uint16_t, std::bitset<32>>,
                                    nvml::obj::allocator<std::tuple<uint16_t, uint16_t, std::bitset<32>>>>;
  p<size_t> numberOfBins;
  p<DimensionUses> dimensions;

 public:
  using ColumnBitsMap = std::map<uint16_t, uint16_t>; //<mapping from column id to number of bits

  BDCCInfo() : numberOfBins(0), dimensions() {}

  explicit BDCCInfo(const ColumnBitsMap &_bitMap) :
    numberOfBins(std::accumulate(_bitMap.begin(), _bitMap.end(), 0,
                                 [](const size_t sum, decltype(*_bitMap.begin()) p) {
                                   return sum + p.second;
                                 })),
    dimensions() { deriveMasks(_bitMap); }

  const auto find(uint16_t item) const {
    for (auto it = dimensions.get_ro().cbegin(); it != dimensions.get_ro().cend(); it++) {
      if (std::get<0>(*it) == item) return it;
    }
    return dimensions.get_ro().cend();
  }

  const auto numBins() const {
    return numberOfBins.get_ro();
  }

  const auto begin() const {
    return dimensions.get_ro().cbegin();
  }

  const auto end() const {
    return dimensions.get_ro().cend();
  }

 private:
  void deriveMasks(ColumnBitsMap colToBits) {
    /* Initialize */
    for (const auto &dim: colToBits) {
      dimensions.get_rw()
        .emplace_back(dim.first,
                      dim.second,
                      std::bitset<32>());
    }

    /* Round robin the bins for mapping */
    auto bdccSize = numBins();
    while (bdccSize > 0) {
      auto i = 0ul;
      for (auto &dim: colToBits) {
        if (std::get<1>(dim)-- > 0) {
          std::get<2>(dimensions.get_rw()[i++])[--bdccSize] = 1;
        }
      }
    }
  }
};/* struct BDCCInfo */

}
} /* namespace pfabric::nvm */

#endif /* PTuple_hpp_ */
