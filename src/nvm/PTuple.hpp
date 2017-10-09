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

#include "core/PFabricTypes.hpp"
#include "core/Tuple.hpp"
#include "nvm/DataNode.hpp"

#include "nvml/include/libpmemobj++/persistent_ptr.hpp"

using nvml::obj::persistent_ptr;
using nvml::obj::p;

namespace pfabric {

namespace nvm {

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
  static T apply(persistent_ptr<BDCC_Block> block, const uint16_t *offsets) {
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
  static std::string apply(persistent_ptr<BDCC_Block> block, const uint16_t *offsets) {
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
struct get_helper<int, ID> {
  static int apply(persistent_ptr<BDCC_Block> block, const uint16_t *offsets) {
    return reinterpret_cast<int&>(block->at(offsets[ID]));
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
  static double apply(persistent_ptr<BDCC_Block> block, const uint16_t *offsets) {
    return reinterpret_cast<double&>(block->at(offsets[ID]));
  }
};

/**************************************************************************//**
 * \brief getAll_helper is a helper function to add all attributes of a PTuple
 *        to a passed TuplePtr instance.
 *
 * \tparam Tuple
 *   the underlying and desired Tuple type of the PTuple
 * \tparam CurrentIndex
 *   the index of the attribute to set next
 *****************************************************************************/
template<class Tuple, std::size_t CurrentIndex>
struct getAll_helper;

/**************************************************************************//**
 * \brief General overload for setting Tuples with more than 1 element to add.
 *
 * This specialization will add the remaining elements first and then set the
 * current attribute value.
 *
 * \tparam Tuple
 *   the underlying and desired Tuple type of the PTuple
 * \tparam CurrentIndex
 *   the index of the attribute to add next
 *****************************************************************************/
template<class Tuple, std::size_t CurrentIndex>
struct getAll_helper {
  static void apply(SmartPtr<Tuple> tptr, persistent_ptr<BDCC_Block> block, const uint16_t* const offsets) {
    getAll_helper<Tuple, CurrentIndex - 1>::apply(tptr, block, offsets);
    auto val = get_helper<typename Tuple::template getAttributeType<CurrentIndex-1>::type, CurrentIndex - 1>::apply(block, offsets);
    tptr->template setAttribute<CurrentIndex-1>(val);
  }
};

/**************************************************************************//**
 * \brief Specialization for setting the first attribute.
 *
 * This specialization will just set the first attribute value.
 *
 * \tparam Tuple
 *    the underlying tuple type having one element
 *****************************************************************************/
template<class Tuple>
struct getAll_helper<Tuple, 1> {
  static void apply(SmartPtr<Tuple> tptr, persistent_ptr<BDCC_Block> block, const uint16_t* const offsets) {
    auto val = get_helper<typename Tuple::template getAttributeType<0>::type, 0>::apply(block, offsets);
    tptr->template setAttribute<0>(val);
  }
};

/**************************************************************************//**
 * \brief PTuplePrinter is a helper function to print a persistent tuple of any
 *        size.
 *
 * PTuplePrinter is a helper function to print a PTuple instance of any size
 * and member types to std::ostream. This template should not be directly used,
 * but only via the Tuple members.
 *
 * \tparam Tuple
 *    the tuple type
 * \tparam CurrentIndex
 *    the index of the attribute value to be printed
 *****************************************************************************/
template<class Tuple, std::size_t CurrentIndex>
struct PTuplePrinter;

/**************************************************************************//**
 * \brief General overload for printing more than 1 element.
 *
 * This specialization will print the remaining elements first and appends the
 * current one after a comma.
 *
 * \tparam Tuple
 *    the underlying tuple type
 * \tparam CurrentIndex
 *    the index of the attribute value to be printed
 *****************************************************************************/
template<class Tuple, std::size_t CurrentIndex>
struct PTuplePrinter {
  static void print(std::ostream& os, persistent_ptr<BDCC_Block> block, const uint16_t* offsets) {
    PTuplePrinter<Tuple, CurrentIndex - 1>::print(os, block, offsets);
    auto val = get_helper<typename Tuple::template getAttributeType<CurrentIndex-1>::type, CurrentIndex - 1>::apply(block, offsets);
    os << "," << val;
  }
};

/**************************************************************************//**
 * \brief Specialization for printing a persistent tuple with 1 element.
 *
 * This specialization will just print the element.
 *
 * \tparam Tuple
 *    the underlying tuple type having one element
 *****************************************************************************/
template<class Tuple>
struct PTuplePrinter<Tuple, 1> {
  static void print(std::ostream& os, persistent_ptr<BDCC_Block> block, const uint16_t* offsets) {
    os << get_helper<typename Tuple::template getAttributeType<0>::type, 0>::apply(block, offsets);
  }
};

/**************************************************************************//**
 * \brief Specialization for printing a persistent tuple with no elements.
 *
 * This specialization will do nothing.
 *
 * \tparam Tuple
 *    the underlying tuple type having no elements
 *****************************************************************************/
template<class Tuple>
struct PTuplePrinter<Tuple, 0> {
  static void print(std::ostream& os, persistent_ptr<BDCC_Block> block, const uint16_t* offsets) {
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
 * persistent_ptr<BDCC_Block> block;
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
    using type = typename Tuple::template getAttributeType<ID>::type;
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
  PTuple(persistent_ptr<BDCC_Block> _block, std::array<uint16_t, NUM_ATTRIBUTES> _offsets) :
      block(_block), offsets(_offsets) {
  }

  PTuple() : block(nullptr), offsets(std::array<uint16_t, Tuple::NUM_ATTRIBUTES>()) {}

  /************************************************************************//**
   * \brief Get a specific attribute value from the persistent tuple.
   *
   * \tparam ID
   *   the index of the requested attribute.
   * \return
   *   a reference to the persistent tuple's attribute with the requested \c ID
   ***************************************************************************/
  template<std::size_t ID>
  typename getAttributeType< ID >::type& getAttribute() {
    auto val = new typename getAttributeType<ID>::type;
    *val = detail::get_helper<typename getAttributeType<ID>::type, ID>::apply(block, offsets.get_ro().data());
    return *val;
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
  inline auto get() {
    return detail::get_helper<typename getAttributeType<ID>::type, ID>::apply(block, offsets.get_ro().data());
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
  const typename getAttributeType< ID >::type& getAttribute() const {
    return detail::get_helper<typename getAttributeType<ID>::type, ID>::apply(block, offsets.get_ro().data());
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
  inline auto get() const {
    return detail::get_helper<typename getAttributeType<ID>::type, ID>::apply(block, offsets.get_ro().data());
  }

  /************************************************************************//**
   * \brief Print this persistent tuple to an ostream.
   *
   * \param[in] os
   *   the output stream to print the tuple
   ***************************************************************************/
  void print(std::ostream& os) {
    detail::PTuplePrinter<Tuple, NUM_ATTRIBUTES>::print(os, block, offsets.get_ro().data());
  }

  /************************************************************************//**
   * \brief Create a new Tuple from this PTuple and return a pointer to it.
   *
   * \return
   *   a smart pointer to the newly created Tuple
   ***************************************************************************/
  SmartPtr<Tuple> createTuple() const {
    typename Tuple::Base tp{};
    SmartPtr<Tuple> tptr(new Tuple(tp));
    detail::getAll_helper<Tuple, NUM_ATTRIBUTES>::apply(tptr, block, offsets.get_ro().data());
    return tptr;
  }

private:

  persistent_ptr<BDCC_Block> block;
  p<std::array<uint16_t, NUM_ATTRIBUTES>> offsets;

}; /* class PTuple */

} /* end namespace nvm */

template<class Tuple>
using PTuplePtr = persistent_ptr<nvm::PTuple<Tuple>>;

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
auto get(const nvm::PTuple<Tuple>& ptp) -> decltype((ptp.template get<ID>())) {
  return ptp.template get<ID>();
}

/**************************************************************************//**
   * \brief Print a persistent tuple to an ostream.
   *
   * \param[in] os
   *   the output stream to print the tuple
 *****************************************************************************/
template<class Tuple>
void print(std::ostream& os, const nvm::PTuple<Tuple>& ptp) {
  ptp.print(os);
}

} /* end namespace pfabric */

/**************************************************************************//**
 * \brief Helper template for printing persistent tuples to an ostream
 *
 * \tparam Tuple
 *   the underlying Tuple of the PTuple
 * \param[in] os
 *   the output stream to print the tuple
 * \param[in] ptp
 *   PTuple instance to print
 * \return
 *   the output stream
 *****************************************************************************/
template<typename Tuple>
std::ostream& operator<<(std::ostream& os, pfabric::nvm::PTuple<Tuple>& ptp) {
  ptp.print(os);
  return os;
}

#endif /* PTuple_hpp_ */
