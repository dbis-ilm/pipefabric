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

#ifndef NVM_Block_hpp_
#define NVM_Block_hpp_

#include <array>

#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"


namespace pfabric {
  namespace nvm {

    using nvml::obj::persistent_ptr;
    using nvml::obj::make_persistent;
    using nvml::obj::delete_persistent;

    /** Positions in NVM_Block */
    const int gDDCRangePos1 = 0;
    const int gDDCRangePos2 = 4;
    const int gCountPos = 8;
    const int gFreeSpacePos = 12;
    const int gSmaOffsetPos = 14;
    const int gDataOffsetPos = 16;

    /** Sizes/Lengths in NVM_Block */
    const int gFixedHeaderSize = 14;
    const int gDDCValueSize = 4;
    const int gAttrOffsetSize = 4;
    const int gOffsetSize = 2;

    /** The size of a single block in persistent memory */
    static constexpr uint16_t gBlockSize = 1 << 15; // 32KB

    /**
     * \brief This type represents a byte array used for persistent structures.
     *
     * A BDCC_Block is a PAX oriented data block with the following structure for 32KB:
     * <ddc_range><ddc_cnt><sma_offset_0><data_offset_0> ...<sma_offset_n><data_offset_n>
     * <sma_min_0><sma_max_0><data_vector_0> ... <sma_min_n><sma_max_n><data_vector_n>
     *  0 ddc_range          -> long (x2) - 8 Byte
     *  8 ddc_cnt            -> long - 4 Byte
     * 12 free_space         -> unsigned short
     * for each attribute:
     * 14 sma_offset_x       -> unsigned short - 2 Byte (depends on block size)
     * 16 data_offset_x      -> unsigned short
     * ...
     *
     * for each attribute (int, double):
     *  . sma_min_x          -> size of attributes data type
     *  . sma_max_x          -> size of attributes data type
     *  . data_vector        -> size of attributes data type * ddc_cnt
     *  ...
     *
     * for each attribute (string - data starts at the end of the minipage):
     *  . sma_min_offset_x   -> unsigned short
     *  . sma_max_offset_x   -> unsigned short
     *  . data_offset_vector -> unsigned short * ddc_cnt
     *  . ...
     *  . data               -> size of all strings + ddc_cnt (Nul termination)
     */
    using BDCC_Block = typename std::array<uint8_t, gBlockSize>;

    template<typename KeyType>
    struct DataNode {
      using KeyVector = std::array<KeyType, 8192>; // <KeyType, nvml::obj::allocator<KeyType>>;

      DataNode() : next(nullptr), block(nullptr) {
        keys = make_persistent<KeyVector>();
      }

      DataNode(BDCC_Block _block) : next(nullptr) {
        block = make_persistent<BDCC_Block>(_block);
        keys = make_persistent<KeyVector>();
      }

      persistent_ptr<struct DataNode> next;
      persistent_ptr<BDCC_Block> block;
      persistent_ptr<KeyVector> keys;

      void clear() {
        if (next) {
          //delete_persistent<struct DataNode>(next);
          next = nullptr;
        }
        if (block) {
          delete_persistent<BDCC_Block>(block);
          block = nullptr;
        }
        delete_persistent<struct DataNode>(this);
      }
    };

  } /* end namespace nvm */
} /* end namespace pfabric */

#endif /* NVM_Block_hpp_ */
