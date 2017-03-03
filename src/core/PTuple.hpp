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

#ifndef PTuple_hpp_
#define PTuple_hpp_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <core/PFabricTypes.hpp>

#include <libpmemobj++/persistent_ptr.hpp>

using nvml::obj::persistent_ptr;

namespace pfabric {

namespace nvm {

/* Positions in NVM_Block */
const int DDCRangePos1  =  0;
const int DDCRangePos2  =  4;
const int CountPos      =  8;
const int FreeSpacePos  = 12;
const int SmaOffsetPos  = 14;
const int DataOffsetPos = 16;

/* Sizes/Lengths in NVM_Block */
const int FixedHeaderSize = 14;
const int AttrOffsetSize  =  4;
const int OffsetSize      =  2;

/** The size of a single block in persistent memory */
static constexpr uint16_t BlockSize = 1 << 15; // 32KB

/**
 * \brief This type represents a byte array used for persistent structures.
 *
 * A NVM_Block is a PAX oriented data block with the following structure for 32KB:
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
typedef typename std::array<uint8_t, BlockSize> NVM_Block;

namespace detail {

/**************************************************************************//**
 * \brief get_helper is a helper function to receive an attribute of a PTuple.
 *
 * \tparam T
 *   the type of the requested attribute
 * \tparam ID
 *   the index of the requested attribute
 *****************************************************************************/
template<typename T, std::size_t ID>
struct get_helper;

/**************************************************************************//**
 * \brief General overload for any type of attribute.
 *
 * \tparam T
 *   the type of the requested attribute
 * \tparam ID
 *   the index of the requested attribute
 *****************************************************************************/
template<typename T, std::size_t ID>
struct get_helper {
  static T apply(persistent_ptr<NVM_Block> block, const std::vector<uint16_t>& offsets) {
    T val;
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&val);
    std::copy(block->begin() + offsets[ID], block->begin() + offsets[ID] + sizeof(T), ptr);
    return val;
  }
};

/**************************************************************************//**
 * \brief Specialization for retrieving an attribute of type string.
 *
 * \tparam T
 *   the type of the requested attribute
 * \tparam ID
 *   the index of the requested attribute
 *****************************************************************************/
template<std::size_t ID>
struct get_helper<std::string, ID> {
  static char (&( apply(persistent_ptr<NVM_Block> block, const std::vector<uint16_t>& offsets)))[] {
    return reinterpret_cast<char (&)[]>(block->at(offsets[ID]));
  }
};

/**************************************************************************//**
 * \brief Specialization for retrieving an attribute of type 32 byte integer.
 *
 * \tparam T
 *   the type of the requested attribute
 * \tparam ID
 *   the index of the requested attribute
 *****************************************************************************/
template<std::size_t ID>
struct get_helper<int32_t, ID> {
  static int32_t& apply(persistent_ptr<NVM_Block> block, const std::vector<uint16_t>& offsets) {
    return reinterpret_cast<int32_t&>(block->at(offsets[ID]));
  }
};

/**************************************************************************//**
 * \brief Specialization for retrieving an attribute of type double.
 *
 * \tparam T
 *   the type of the requested attribute
 * \tparam ID
 *   the index of the requested attribute
 *****************************************************************************/
template<std::size_t ID>
struct get_helper<double, ID> {
  static double& apply(persistent_ptr<NVM_Block> block, const std::vector<uint16_t>& offsets) {
    return reinterpret_cast<double&>(block->at(offsets[ID]));
  }
};

} /* end namespace detail */

/**************************************************************************//**
 * \brief A persistent Tuple used for referencing tuples in a persistent table.
 *
 * A PTuple consist of a persistent pointer to the \c block where the
 * underlying tuple is stored. The \c offsets are used to locate the individual
 * attributes of the tuple within the \c block.
 *
 * \code
 * persistent_ptr<NVM_Block> block;
 * std::vector<uint16_t> tupleOffsets;
 *
 * // Insert into block and save the offsets ...
 *
 * PTuple<pfabric::Tuple<int, double, std::string>> ptp(block, tupleOffsets);
 * \endcode
 *
 * Get reference to single attribute:
 *
 * \code
 * auto attr1 = ptp.template get<0>;
 * // or:
 * auto attr1 = get<0>(ptp);
 * \endcode
 *
 * \note string attributes are returned as reference to a char array
 * \author Philipp Goetze <philipp.goetze@tu-ilmenau.de>
 *****************************************************************************/
template<class Tuple>
class PTuple {

private:

  persistent_ptr<NVM_Block> block;
  std::vector<uint16_t> offsets;//!< TODO: Something else than a runtime eater like vector

public:
  /************************************************************************//**
   * \brief the number of attributes for this tuple type.
   ***************************************************************************/
  static const TupleSize NUM_ATTRIBUTES = Tuple::NUM_ATTRIBUTES;

  /************************************************************************//**
   * \brief Meta function returning the type of a specific tuple attribute.
   *
   * \tparam ID
   *   the index of the requested attribute.
   ***************************************************************************/
  template<AttributeIdx ID>
  struct getAttributeType {
    typedef typename Tuple::template getAttributeType<ID>::type type;
  };

  /************************************************************************//**
   * \brief Constructs a new persistent tuple using a persistent block and
   *        offsets for the tuple elements.
   *
   * \tparam Tuple
   *   the underlying tuple type used as base
   * \param[in] _block
   *   the persistent block containing the tuple data (bytes)
   * \param[in] _offsets
   *   the offsets for each tuple element
   ***************************************************************************/
  PTuple(persistent_ptr<NVM_Block> _block, std::vector<uint16_t> _offsets) :
      block(_block), offsets(_offsets) {
  }

  /************************************************************************//**
   * \brief Get a specific attribute value from the persistent tuple.
   *
   * \tparam ID
   *   the index of the requested attribute.
   * \return
   *   a reference to the persistent tuple's attribute with the requested \c ID
   ***************************************************************************/
  template<std::size_t ID>
  inline auto getAttribute() -> typename getAttributeType<ID>::type {
    return detail::get_helper<typename getAttributeType<ID>::type, ID>::apply(block, offsets);
  }

  template<std::size_t ID>
  inline auto get() -> typename getAttributeType<ID>::type {
    return detail::get_helper<typename getAttributeType<ID>::type, ID>::apply(block, offsets);
  }

}; /* class PTuplePtr */

} /* end namespace nvm */

/**************************************************************************//**
 * \brief Get a specific attribute reference from the PTuple.
 *
 * A global accessor function to reduce boilerplate code that needs to be
 * written to access a specific attribute of a PTuple.

 * \tparam ID
 *   the index of the requested attribute.
 * \tparam Tuple
 *   the underlying tuple type used as base
 * \param[in] ptp
 *   the persistent tuple (PTuple) instance
 * \return
 *   a reference to the persistent tuple's attribute with the requested \c ID
 *****************************************************************************/
template<std::size_t ID, class Tuple>
auto get(nvm::PTuple<Tuple> ptp) -> decltype(ptp.template get<ID>()) {
  return ptp.template get<ID>();
}

} /* end namespace pfabric */

#endif /* PTuple_hpp_ */
