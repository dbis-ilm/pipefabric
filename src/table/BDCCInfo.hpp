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

namespace pfabric {

/**************************************************************************//**
 * \brief Info structure about the BDCC meta data.
 *
 * It is used in persistent tables to store the BDCC meta data and statistics.
 *****************************************************************************/
struct BDCCInfo {
  typedef std::unordered_map<ColumnInfo, uint16_t> ColumnBitsMap;

  BDCCInfo(ColumnBitsMap _bitMap) : bitMap(_bitMap),
      numBins(std::accumulate(_bitMap.begin(), _bitMap.end(), 0,
      [](const size_t sum, decltype(*_bitMap.begin()) p) { return sum + p.second; })) {}

  const ColumnBitsMap bitMap;
  const size_t numBins;
  std::map<uint32_t, std::size_t> histogram;

};/* struct BDCCInfo */

} /* namespace pfabric */

#endif /* PTuple_hpp_ */
