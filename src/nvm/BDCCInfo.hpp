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

#include <nvm/PTableInfo.hpp>
#include <map>
#include <unordered_map>
#include <algorithm>

#include "nvml/include/libpmemobj++/allocator.hpp"
#include "nvml/include/libpmemobj++/detail/persistent_ptr_base.hpp"
#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/transaction.hpp"
#include "nvml/include/libpmemobj++/utils.hpp"

namespace pfabric {
  namespace nvm {

    using nvml::obj::allocator;
    using nvml::obj::delete_persistent;
    using nvml::obj::make_persistent;
    using nvml::obj::p;
    using nvml::obj::persistent_ptr;
    using nvml::obj::pool_by_vptr;
    using nvml::obj::transaction;

/**************************************************************************//**
 * \brief Info structure about the BDCC meta data.
 *
 * It is used in persistent tables to store the BDCC meta data and statistics.
 *****************************************************************************/
    class BDCCInfo {
      using pColumnBitsMap = const std::vector<std::pair<uint16_t, uint16_t>, nvml::obj::allocator<std::pair<uint16_t, uint16_t>>>;
      public:
      using ColumnBitsMap = const std::unordered_map<uint16_t, uint16_t>; //<mapping from column id to number of bits
      explicit BDCCInfo(const ColumnBitsMap &_bitMap) :
        bitMap(_bitMap.cbegin(), _bitMap.cend()),
        numberOfBins(std::accumulate(_bitMap.begin(), _bitMap.end(), 0,
                                [](const size_t sum, decltype(*_bitMap.begin()) p) { return sum + p.second; })) {}

      const pColumnBitsMap::const_iterator find(uint16_t item) const {
        for (auto it = bitMap.cbegin(); it != bitMap.cend(); it++) {
          if (it->first == item) return it;
        }
        return bitMap.cend();
      }

      const size_t numColumns() const {
        return bitMap.size();
      }

      const size_t numBins() const {
        return numberOfBins.get_ro();
      }

      const pColumnBitsMap::const_iterator cend() const {
        return bitMap.cend();
      }

    //  private:
      const pColumnBitsMap bitMap;
      p<const size_t> numberOfBins;
      //std::map<uint32_t, std::size_t> histogram;
    };/* struct BDCCInfo */

  }
} /* namespace pfabric::nvm */

#endif /* PTuple_hpp_ */
