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

#ifndef persistent_table_hpp_
#define persistent_table_hpp_

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/serialize.hpp"
#include "nvm/BDCCInfo.hpp"
#include "nvm/ctree_map_persistent.hpp"
#include "nvm/PTuple.hpp"
#include "table/TableException.hpp"
#include "table/TableInfo.hpp"

#include "nvml/include/libpmemobj++/detail/persistent_ptr_base.hpp"
#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/pool.hpp"
#include "nvml/include/libpmemobj++/transaction.hpp"
#include "nvml/include/libpmemobj++/utils.hpp"

namespace pfabric { namespace nvm {

using nvml::obj::pool;
using nvml::obj::pool_by_vptr;
using nvml::obj::pool_base;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::make_persistent;
using nvml::obj::delete_persistent;
using nvml::obj::transaction;

const std::string LAYOUT = "PTable";

/**************************************************************************//**
 * \brief A persistent table used for PMEM technologies or emulations.
 *
 * \author Philipp Goetze <philipp.goetze@tu-ilmenau.de>
 *****************************************************************************/
template<class Tuple, typename KeyType>
class persistent_table {

public:
  typedef persistent_ptr<Tuple> RecordType;
  typedef std::unordered_map<uint16_t, uint16_t> ColumnIntMap;


  /************************************************************************//**
   * \brief Default Constructor.
   ***************************************************************************/
  persistent_table() {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {init("", {}, ColumnIntMap());});
  }

  /************************************************************************//**
   * \brief Constructor for a given schema (TableInfo).
   ***************************************************************************/
  persistent_table(const std::string& tName, ColumnInitList columns) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {init(tName, columns, ColumnIntMap());});
  }

  /************************************************************************//**
   * \brief Constructor for a given schema and dimension clustering.
   ***************************************************************************/
  persistent_table(const std::string& tName, ColumnInitList columns, const ColumnIntMap& _bdccInfo) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {init(tName, columns, _bdccInfo);});
  }

  /************************************************************************//**
   * \brief Default Destructor.
   ***************************************************************************/
  ~persistent_table() {
    //TODO: Complete this
    root->block_list->clear();
  }

  /************************************************************************//**
   * \brief Inserts a new record into the persistent table to the fitting
   *        nvm_block.
   *
   * \param[in] rec
   *   the new record/tuple to insert.
   * \return
   *   number of inserted tuples
   ***************************************************************************/
  int insert(KeyType key, Tuple rec) {

    /* Block across implementation
     *
     * X Calc bdcc value
     * X Search correct block
     * X Check for enough space
     *   - yes, but not in minipage -> rearrange (average) <-- TODO:
     *   X no -> split block (what if bdcc range only one value?)
     * X Insert + adapt SMAs and count
     */

    //TODO: Check if already inserted in Index
    auto dest_block = root->block_list;
    StreamType buf;
    rec.serializeToStream(buf);

    /* Calculate BDCC value for input tuple*/
    auto xtr = static_cast<uint32_t>(getBDCCFromTuple(rec).to_ulong());

    /* Search for correct Block */
    do {
      /* Retrieve DDC Range */
      const auto& ddc_min = reinterpret_cast<const uint32_t &>(dest_block->block->at(nvm::gDDCRangePos1));
      const auto& ddc_max = reinterpret_cast<const uint32_t &>(dest_block->block->at(nvm::gDDCRangePos2));

      if (xtr >= ddc_min && xtr <= ddc_max) break; //Found correct block

      auto cur_block = dest_block->next;
      dest_block = cur_block;
    } while (dest_block != nullptr); /* Should not reach end! */

    /* Check for enough space in target block */
    auto& space = reinterpret_cast<uint16_t&>(dest_block->block->at(nvm::gFreeSpacePos));
    if (space < buf.size()) {
      auto newBlocks = splitBlock(dest_block);
      const auto& splitValue = reinterpret_cast<uint32_t &>(newBlocks.first->block->at(nvm::gDDCRangePos2));
      auto xtr = static_cast<uint32_t>(getBDCCFromTuple(rec).to_ulong());
      if (xtr <= splitValue)
        return insertTuple(key, rec, newBlocks.first);
      else
        return insertTuple(key, rec, newBlocks.second);
    }

    return insertTuple(key, rec, dest_block);
  }

  /************************************************************************//**
   * \brief Update a specific attribute of a tuple specified by the given key.
   *
   * Update the complete tuple in the table associated with the given key. For
   * that the tuple is deleted first and then newly inserted.
   *
   * \param[in] key
   *   the key value
   * \param[in] pos
   *   the index of the tuple's attribute
   * \param[in] rec
   *   the new tuple values for this key
   * \return
   *   the number of modified tuples
   ***************************************************************************/
  int updateAttribute(KeyType key, size_t pos, RecordType rec) {
    //TODO: Implement. But is this really necessary?
    return 0;
  }

  /************************************************************************//**
   * \brief Update the complete tuple specified by the given key.
   *
   * Update the complete tuple in the table associated with the given key. For
   * that the tuple is deleted first and then newly inserted.
   *
   * \param[in] key
   *   the key value
   * \param[in] rec
   *   the new tuple values for this key
   * \return
   *   the number of modified tuples
   ***************************************************************************/
  int updateComplete(KeyType key, RecordType rec) {
    auto nres = deleteByKey(key);
    if (!nres)
      throw TableException("key not found");
    nres = insert(key, rec);
    return nres;
  }

  /************************************************************************//**
   * \brief Delete the tuple from the table associated with the given key.
   *
   * Delete the tuple from the persistent table and index structure that is
   * associated with the given key.
   *
   * \param[in] key
   *   the key value
   * \return
   *   the number of tuples deleted
   ***************************************************************************/
  int deleteByKey(KeyType key) {
    //TODO: Also delete data?
    size_t nres;
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      //delete_persistent<PTuple<Tuple>>(getByKey(key));
      nres = root->index->remove_free(key);
    });
    return nres;
  }

  /************************************************************************//**
   * \brief Return the PTuple associated with the given key.
   *
   * Return the tuple from the persistent table that is associated with the
   * given key. If the key doesn't exist, an exception is thrown.
   *
   * \param[in] key
   *   the key value
   * \return
   *   the PTuple associated with the given key
   ***************************************************************************/
  persistent_ptr<PTuple<Tuple>> getByKey(KeyType key) const {
    auto map = *root->index;
    //if (res != root->index->end()) {
    if (map.lookup(key)) {
      return *map.get(key);
    } else {
      throw TableException("key not found");
    }
  }

  /************************************************************************//**
   * \brief Prints the table content column-wise.
   *
   * \param[in] raw
   *   set to true to additionaly print out the complete raw byte arrays.
   ***************************************************************************/
  void print(bool raw = false) {

    auto dest_block = root->block_list;
    auto& tInfo = *root->tInfo;

    size_t colCnt = 0;
    for (auto &c : tInfo)
      colCnt++;

    do {
      auto b = dest_block->block;

      const auto& key1 = reinterpret_cast<const uint32_t&>(b->at(nvm::gDDCRangePos1));
      const auto& key2 = reinterpret_cast<const uint32_t&>(b->at(nvm::gDDCRangePos2));
      const auto& cnt = reinterpret_cast<const uint32_t&>(b->at(nvm::gCountPos));
      const auto& space = reinterpret_cast<const uint16_t&>(b->at(nvm::gFreeSpacePos));
      const auto headerSize = nvm::gFixedHeaderSize + nvm::gAttrOffsetSize * colCnt;
      const auto bodySize = nvm::gBlockSize - headerSize;

      /* Plain byte-by-byte output */
      if (raw) {
        size_t i = 0;
        printf("[ ");
        for (auto &byte : *b) {
          printf("%02x ", byte);
          if (++i % 32 == 0) {
            printf("]");
            if (i < b->size())
              printf("\n[ ");
          }
        }
      }

      /* Header/General information */
      std::cout << "\nDDC Range min: " << key1 << '\n'
                << "DDC Range max: " << key2 << '\n'
                << "Tuple count: " << cnt << '\n'
                << "Header size: " << headerSize << " Bytes" << '\n'
                << "Body size: " << bodySize << " Bytes" << '\n'
                << "Free Space: " << space << " Bytes" << std::endl;

      /* Body/Column/Minipage data */
      if (cnt > 0) {
        size_t idx = 0;
        for (auto &c : tInfo) {
          std::cout << "Column Info: " << c.getName() << ": " << c.getType() << std::endl;
          const auto& sma_pos = reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + idx * nvm::gAttrOffsetSize));
          const auto& data_pos = reinterpret_cast<const uint16_t&>(b->at(nvm::gDataOffsetPos + idx * nvm::gAttrOffsetSize));

          switch (c.getType()) {
          case ColumnInfo::Int_Type: {
            const auto& sma_min = reinterpret_cast<const int32_t&>(b->at(sma_pos));
            const auto& sma_max =reinterpret_cast<const int32_t&>(b->at(sma_pos + sizeof(int32_t)));
            const auto& data = reinterpret_cast<const int32_t(&)[cnt]>(b->at(data_pos));

            /* Remaining Space */
            auto next_sma_pos = (colCnt == idx + 1) ?
              nvm::gBlockSize :
              reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx + 1) * nvm::gAttrOffsetSize));
            auto mp_free = next_sma_pos - data_pos - (cnt * sizeof(int32_t));

            std::cout << "Column[" << idx << "]: " << c.getName()
                      << "\n\tSpace left: " << mp_free << " Bytes"
                      << "\n\tsma_min: " << sma_min
                      << "\n\tsma_max: " << sma_max
                      << "\n\tData: {";
            const char *padding = "";
            for (auto i = 0u; i < cnt; i++) {
              std::cout << padding << data[i];
              padding = ", ";
            }
            std::cout << "}\n";
          } break;

          case ColumnInfo::Double_Type: {
            const auto& sma_min = reinterpret_cast<const double&>(b->at(sma_pos));
            const auto& sma_max = reinterpret_cast<const double&>(b->at(sma_pos + sizeof(double)));
            const auto& data = reinterpret_cast<const double(&)[cnt]>(b->at(data_pos));

            /* Remaining Space */
            auto next_sma_pos =(colCnt == idx + 1) ?
              nvm::gBlockSize :
              reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx + 1) * nvm::gAttrOffsetSize));
            uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(double));

            std::cout << "Column[" << idx << "]: " << c.getName()
                      << "\n\tSpace left: " << mp_free << " Bytes"
                      << "\n\tsma_min: " << sma_min
                      << "\n\tsma_max: " << sma_max
                      << "\n\tData: {";
            const char *padding = "";
            for (auto i = 0u; i < cnt; i++) {
              std::cout << padding << data[i];
              padding = ", ";
            }
            std::cout << "}\n" << std::endl;
          } break;

          case ColumnInfo::String_Type: {
            auto &sma_min_pos = (uint16_t &) b->at(sma_pos);
            auto &sma_max_pos = (uint16_t &) b->at(sma_pos + nvm::gOffsetSize);
            auto &string_pos = (uint16_t (&)[cnt]) b->at(data_pos);
            auto sma_min(reinterpret_cast<const char (&)[]>(b->at(sma_min_pos)));
            auto sma_max(reinterpret_cast<const char (&)[]>(b->at(sma_max_pos)));

            auto current_offset_pos = data_pos + cnt * nvm::gOffsetSize;
            auto current_offset = reinterpret_cast<const uint16_t&>(b->at(current_offset_pos - nvm::gOffsetSize));
            auto mp_free = current_offset - current_offset_pos;

            std::cout << "Column[" << idx << "]: " << c.getName()
                      << "\n\tSpace left: " << mp_free << " Bytes"
                      << "\n\tsma_min: " << sma_min
                      << "\n\tsma_max: " << sma_max
                      << "\n\tData: {";
            const char *padding = "";
            for (auto i = 0u; i < cnt; i++) {
              std::string data(reinterpret_cast<char (&)[]>(b->at(string_pos[i])));
              std::cout << padding << data;
              padding = ", ";
            }
            std::cout << "}\n" << std::endl;
          } break;

          default: {
            throw TableException("unsupported column type\n");
          } break;
          }
          ++idx;
        } /* end for (auto &c: tInfo) */
      } /* if cnt > 0 */

      auto cur_block = dest_block->next;
      dest_block = cur_block;
    } while (dest_block != nullptr);
  }

// Private ////////////////////////////////////////////////////////////////////
private:
  struct nvm_block {
    nvm_block() :
        next(nullptr), block(nullptr) {
    }
    nvm_block(nvm::NVM_Block _block) :
        next(nullptr), block(_block) {
    }

    persistent_ptr<struct nvm_block> next;
    persistent_ptr<nvm::NVM_Block> block;

    void clear() {
      if (next) {
        delete_persistent<struct nvm_block>(next);
        next = nullptr;
      }
      if (block) {
        delete_persistent<nvm::NVM_Block>(block);
        block = nullptr;
      }
    }
  };

  struct root {
    persistent_ptr<struct nvm_block> block_list;
    persistent_ptr<examples::ctree_map_p<KeyType, persistent_ptr<nvm::PTuple<Tuple>>>> index;
    persistent_ptr<const TableInfo> tInfo;
    persistent_ptr<BDCCInfo> bdccInfo;
  };
  persistent_ptr<struct root> root;



  /**************************************************************************//**
   * \brief Helper function to calculate the minipage sizes for a given schema.
   *
   * \param[in] totalSize
   *   total available space/size
   * \param[in] customizations
   *   optional map of weightings for specific columns.
   * \return
   *   a mapping from ColumnInfo to the calculated minipage size
   *****************************************************************************/
   ColumnIntMap calcMinipageSizes(uint16_t totalSize, ColumnIntMap customizations = ColumnIntMap()) {
      size_t portions = 0;
      ColumnIntMap mp_sizes = ColumnIntMap();
      /* Get the sum of all column portitions */
      for (auto i = 0u; i < (*root->tInfo).numColumns(); i++) {
        const auto &c = (*root->tInfo).columnInfo(i);
        if (customizations.find(i) == customizations.end()) {
          switch (c.getType()) {
          case ColumnInfo::Int_Type:
            portions += 1;
            break;
          case ColumnInfo::Double_Type:
            portions += 2;
            break;
          case ColumnInfo::String_Type:
            portions += 5;
            break;
          default:
            throw TableException("unsupported column type\n");
            break;
          }
        } else
          portions += customizations[i];
      }
      /* Calculate and save the minipage sizes for all columns */
      for (auto i = 0u; i < (*root->tInfo).numColumns(); i++) {
        const auto &c = (*root->tInfo).columnInfo(i);
        if (customizations.find(i) == customizations.end()) {
          switch (c.getType()) {
          case ColumnInfo::Int_Type:
            mp_sizes[i] = 1 * totalSize / portions;
            break;
          case ColumnInfo::Double_Type:
            mp_sizes[i] = 2 * totalSize / portions;
            break;
          case ColumnInfo::String_Type:
            mp_sizes[i] = 5 * totalSize / portions;
            break;
          default:
            throw TableException("unsupported column type\n");
            break;
          }
        } else
          mp_sizes[i] = customizations[i] * totalSize / portions;
      }
      return mp_sizes;
    }


  /************************************************************************//**
   * \brief Initialization function for creating the necessary structures.
   *
   * \param[in] _tInfo
   *   the underlying schema to use
   ***************************************************************************/
  void init(const std::string& _tName, const ColumnInitList& _columns, const ColumnIntMap& _bdccInfo) {
    this->root = make_persistent<struct root>();
    this->root->tInfo = make_persistent<TableInfo>(_tName, _columns);
    this->root->bdccInfo = make_persistent<BDCCInfo>(_bdccInfo);
    this->root->index = make_persistent<examples::ctree_map_p<KeyType, persistent_ptr<nvm::PTuple<Tuple>>>>();
    this->root->block_list = make_persistent<struct nvm_block>();
    auto block_ptr = make_persistent<nvm::NVM_Block>(initBlock(0, ((1L << this->root->bdccInfo->numBins) - 1)));
    this->root->block_list->block = block_ptr;
  }

  /************************************************************************//**
   * \brief Initialize a new nvm::NVM_Block.
   *
   * \return a new initialized nvm::NVM_Block.
   ***************************************************************************/
  nvm::NVM_Block initBlock(const uint32_t& ddc0, const uint32_t& ddc1) {
    auto b = nvm::NVM_Block{};

    std::copy(reinterpret_cast<const uint8_t *>(&ddc0),
              reinterpret_cast<const uint8_t *>(&ddc0) + nvm::gDDCValueSize,
              b.begin() + nvm::gDDCRangePos1);
    std::copy(reinterpret_cast<const uint8_t *>(&ddc1),
              reinterpret_cast<const uint8_t *>(&ddc1) + nvm::gDDCValueSize,
              b.begin() + nvm::gDDCRangePos2);

    auto colCnt = 0;
    for (auto &c : *this->root->tInfo)
      colCnt++;

    uint16_t header_size = nvm::gFixedHeaderSize + colCnt * nvm::gAttrOffsetSize;
    uint16_t body_size = nvm::gBlockSize - header_size;
    auto minipage_size = body_size / colCnt;

    auto sizes = calcMinipageSizes(body_size);

    /* Set Offsets */
    size_t idx = 0;
    size_t sma_size = 0;
    uint16_t current_offset = header_size;
    for (auto i = 0u; i < (*this->root->tInfo).numColumns(); i++) {
      const auto &c = (*this->root->tInfo).columnInfo(i);
      uint16_t sma_offset;
      uint16_t data_offset;
      switch (c.getType()) {
      case ColumnInfo::Int_Type: {
        sma_offset = current_offset;
        data_offset = current_offset + 2 * sizeof(uint32_t);
        sma_size += 2 * sizeof(uint32_t);
        current_offset += sizes[i];
      }
        break;
      case ColumnInfo::Double_Type: {
        sma_offset = current_offset;
        data_offset = current_offset + 2 * sizeof(uint64_t);
        sma_size += 2 * sizeof(uint64_t);
        current_offset += sizes[i];
      }
        break;
      case ColumnInfo::String_Type: {
        sma_offset = current_offset;
        data_offset = current_offset + nvm::gAttrOffsetSize;
        sma_size += nvm::gAttrOffsetSize;
        current_offset += sizes[i];
      }
        break;
      default: {
        throw TableException("unsupported column type\n");
      }
        break;
      }

      std::copy(reinterpret_cast<const uint8_t *>(&sma_offset),
                reinterpret_cast<const uint8_t *>(&sma_offset) + nvm::gOffsetSize,
                b.begin() + (nvm::gSmaOffsetPos + idx * nvm::gAttrOffsetSize));
      std::copy(reinterpret_cast<const uint8_t *>(&data_offset),
                reinterpret_cast<const uint8_t *>(&data_offset) + nvm::gOffsetSize,
                b.begin() + (nvm::gDataOffsetPos + idx * nvm::gAttrOffsetSize));
      ++idx;
    }

    /* Set Free Space field */
    uint16_t free_space = body_size - sma_size;
    std::copy(reinterpret_cast<const uint8_t *>(&free_space),
              reinterpret_cast<const uint8_t *>(&free_space) + sizeof(uint16_t),
              b.begin() + nvm::gFreeSpacePos);

    return b;
  }

  int insertTuple(KeyType key, Tuple rec, persistent_ptr<struct nvm_block>& dest_block) {
    auto pop = pool_by_vptr(this);
    auto b = dest_block->block;
    auto tInfo = *this->root->tInfo;
    auto record_size = 0;
    size_t idx = 0;
    size_t recOffset = 1;
    std::array<uint16_t, Tuple::NUM_ATTRIBUTES> pTupleOffsets;
    StreamType buf;
    rec.serializeToStream(buf);
    auto& cnt = reinterpret_cast<uint32_t&>(b->at(nvm::gCountPos));

    try {
    transaction::exec_tx(pop, [&] {
      // Each attribute (SMA + Data)
        for (auto &c : tInfo) {
          const auto& sma_pos = reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + idx * nvm::gAttrOffsetSize));
          const auto& data_pos = reinterpret_cast<const uint16_t&>(b->at(nvm::gDataOffsetPos + idx * nvm::gAttrOffsetSize));

          switch (c.getType()) {

            case ColumnInfo::Int_Type: {
              /* Get Record Value */
              auto it_begin = buf.cbegin() + recOffset;
              auto it_end = it_begin + sizeof(int32_t);
              int32_t value = deserialize<int32_t>(it_begin, it_end);

              /* Check remaining space in minipage */
              auto next_sma_pos = (rec.size()==idx+1) ? nvm::gBlockSize :
              reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx+1) * nvm::gAttrOffsetSize));
              uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(int32_t));
              if(mp_free < sizeof(int32_t)) {
                std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
                transaction::abort(100);
              }

              /* Insert Data */
              uint16_t data_offset = data_pos + cnt * sizeof(int32_t);
              std::copy(reinterpret_cast<const uint8_t *>(&value),
                  reinterpret_cast<const uint8_t *>(&value) + sizeof(int32_t),
                  b->begin() + data_offset);

              /* Update SMA */
              auto& sma_min = reinterpret_cast<int32_t&>(b->at(sma_pos));
              auto& sma_max = reinterpret_cast<int32_t&>(b->at(sma_pos + sizeof(int32_t)));
              if (sma_min > value || cnt == 0) sma_min = value;
              if (sma_max < value || cnt == 0) sma_max = value;

              /* Set new positions and sizes */
              recOffset += sizeof(int32_t);
              record_size += sizeof(int32_t);
              pTupleOffsets[idx] = data_offset;

            }break;

            case ColumnInfo::Double_Type: {
              /* Get Record Value */
              auto it_begin = buf.cbegin() + recOffset;
              auto it_end = it_begin + sizeof(double);
              double value = deserialize<double>(it_begin, it_end);

              /* Check remaining space in minipage */
              uint16_t next_sma_pos;
              if (rec.size()==idx+1) {
                next_sma_pos = nvm::gBlockSize;
              } else {
                next_sma_pos = reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx+1) * nvm::gAttrOffsetSize));
              }
              uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(double));
              if(mp_free < sizeof(double)) {
                std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
                transaction::abort(100);
              }

              /* Insert Data */
              uint16_t data_offset = data_pos + cnt * sizeof(double);
              std::copy(reinterpret_cast<const uint8_t *>(&value),
                  reinterpret_cast<const uint8_t *>(&value) + sizeof(double),
                  b->begin() + data_offset);

              /* Update SMA */
              auto& sma_min = reinterpret_cast<double&>(b->at(sma_pos));
              auto& sma_max = reinterpret_cast<double&>(b->at(sma_pos + sizeof(double)));
              if (sma_min > value || cnt == 0) sma_min = value;
              if (sma_max < value || cnt == 0) sma_max = value;

              /* Set new positions and sizes */
              recOffset += sizeof(double);
              record_size += sizeof(double);
              pTupleOffsets[idx] = data_offset;

            }break;

            case ColumnInfo::String_Type: {
              /* Get Record Value */
              auto it_begin = buf.cbegin() + recOffset;
              auto value = deserialize<std::string>(it_begin, buf.end());
              const char* c_value = value.c_str();
              uint64_t string_size = value.size() + 1;

              /* Check remaining space in minipage */
              uint16_t mp_free;
              if (cnt != 0) {
                uint16_t current_offset_pos = data_pos + cnt * nvm::gOffsetSize;
                const auto& current_offset = reinterpret_cast<const uint16_t&>(b->at(current_offset_pos - nvm::gOffsetSize));
                mp_free = current_offset - current_offset_pos;
              } else {
                uint16_t next_sma_pos;
                if (rec.size()==idx+1) {
                  next_sma_pos = nvm::gBlockSize;
                } else {
                  next_sma_pos = reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx+1) * nvm::gAttrOffsetSize));
                }
                mp_free = next_sma_pos - data_pos;
              }
              if(mp_free < string_size + nvm::gOffsetSize) {
                std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
                transaction::abort(100);
              }

              /* Insert Data - Get target position */
              uint16_t target_offset_pos = data_pos + cnt * nvm::gOffsetSize;
              uint16_t target_data_pos;
              if (cnt == 0) {
                auto end_minipage = (idx < rec.size() - 1) ?
                reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx + 1) * nvm::gAttrOffsetSize)) : nvm::gBlockSize;
                target_data_pos = end_minipage - string_size;
              } else /* cnt != 0 */{
                const auto& last_offset = reinterpret_cast<const uint16_t&>(b->at(target_offset_pos - nvm::gOffsetSize));
                target_data_pos = last_offset - string_size;
              }

              /* Insert Data - Set offset and string data */
              std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                  reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::gOffsetSize,
                  b->begin() + target_offset_pos);
              std::copy(reinterpret_cast<const uint8_t *>(c_value),
                  reinterpret_cast<const uint8_t *>(c_value) + string_size,
                  b->begin() + target_data_pos);

              /* Update SMA */
              auto& sma_min_pos = reinterpret_cast<uint16_t &>(b->at(sma_pos));
              auto& sma_max_pos = reinterpret_cast<uint16_t &>(b->at(sma_pos + nvm::gOffsetSize));
              if (cnt != 0) {
                std::string sma_min(reinterpret_cast<const char (&)[]>(b->at(sma_min_pos)));
                std::string sma_max(reinterpret_cast<const char (&)[]>(b->at(sma_max_pos)));
                if (sma_min > value) sma_min_pos = target_data_pos;
                else if (sma_max < value) sma_max_pos =target_data_pos;
              } else /* cnt == 0 */{
                sma_min_pos = target_data_pos;
                sma_max_pos = target_data_pos;
              }

              /* Set new positions and sizes */
              recOffset += string_size - 1 + sizeof(uint64_t);
              record_size += string_size + nvm::gOffsetSize;
              pTupleOffsets[idx] = target_data_pos;

            }break;

            default: {
              throw TableException("unsupported column type\n");
            }break;
          } /* switch (c.getType()) */
          ++idx;
        } /* for (auto &c : tInfo) */

        /* Increase BDCC count */
        ++reinterpret_cast<uint32_t&>(b->at(nvm::gCountPos));
        /* Adapt Free Space */
        auto& fspace = reinterpret_cast<uint16_t&>(b->at(nvm::gFreeSpacePos));
        fspace -= record_size;

        /* Insert into index structure */
        auto ptp = make_persistent<PTuple<Tuple>>(b, pTupleOffsets);
        root->index->insert_new( key, ptp );
      }); /* end of transaction */

    } catch (std::exception& e) {
      //TODO: only catch abort
      std::cout << e.what() << '\n';
      auto newBlocks = splitBlock(dest_block);
      const auto& splitValue = reinterpret_cast<uint32_t &>(newBlocks.first->block->at(nvm::gDDCRangePos2));
      auto xtr = static_cast<uint32_t>(getBDCCFromTuple(rec).to_ulong());
      return insertTuple(key, rec, (xtr <= splitValue) ? newBlocks.first : newBlocks.second);
    }

    return 1;
  }

  std::pair<persistent_ptr<struct nvm_block>, persistent_ptr<struct nvm_block>> splitBlock(
      persistent_ptr<struct nvm_block>& nvmBlock) {
    auto pop = pool_by_vptr(this);
    auto tInfo = *this->root->tInfo;
    auto b0 = nvmBlock->block;

    /* Calculate new ranges from histogram (at half for the beginning) */
    const auto& ddc_min = reinterpret_cast<uint32_t &>(b0->at(nvm::gDDCRangePos1));
    const auto& ddc_max = reinterpret_cast<uint32_t &>(b0->at(nvm::gDDCRangePos2));
    //TODO: Special Case ddc_min == ddc_max
    uint32_t splitValue = ddc_min + (ddc_max - ddc_min) / 2;
    std::cout << "Splitting at: " << splitValue << " (" << ddc_min << ", " << ddc_max << ")\n";

    persistent_ptr<struct nvm_block> newBlock1;
    persistent_ptr<struct nvm_block> newBlock2;

    /* Create two new blocks */
    transaction::exec_tx(pop,
        [&] {
          newBlock1 = make_persistent<struct nvm_block>();
          newBlock2 = make_persistent<struct nvm_block>();
          newBlock1->block = make_persistent<nvm::NVM_Block>(nvm::NVM_Block(initBlock(ddc_min, splitValue)));
          newBlock2->block = make_persistent<nvm::NVM_Block>(nvm::NVM_Block(initBlock(splitValue + 1, ddc_max)));
          auto b1 = newBlock1->block;
          auto b2 = newBlock2->block;

          /* Get, Calculate BDCC, Insert and delete all current values to corresponding block */
          // TODO: what about deleted records?
          auto& cnt = reinterpret_cast<uint16_t&>(b0->at(nvm::gCountPos));
          for (auto tupleOffset = 0u; tupleOffset < cnt; tupleOffset++) {
            auto record_size = 0;
            std::array<uint16_t, Tuple::NUM_ATTRIBUTES> pTupleOffsets;
            // get BDCC value for tuple and decide for block

            auto b = b1;

            auto& dest_cnt = reinterpret_cast<uint32_t&>(b->at(nvm::gCountPos));

            auto idx = 0u;
            for (auto &c : tInfo) {
              const auto& sma_pos = reinterpret_cast<const uint16_t&>(b0->at(nvm::gSmaOffsetPos + idx * nvm::gAttrOffsetSize));
              const auto& data_pos = reinterpret_cast<const uint16_t&>(b0->at(nvm::gDataOffsetPos + idx * nvm::gAttrOffsetSize));

              switch (c.getType()) {

                case ColumnInfo::Int_Type: {
                  /* Get Record Value */
                  uint16_t src_data_offset = data_pos + tupleOffset * sizeof(int32_t);
                  const auto& value = reinterpret_cast<const int32_t&>(b0->at(src_data_offset));

                  /* Insert Data */
                  uint16_t dest_data_offset = data_pos + dest_cnt * sizeof(int32_t);
                  std::copy(reinterpret_cast<const uint8_t *>(&value),
                            reinterpret_cast<const uint8_t *>(&value) + sizeof(int32_t),
                            b->begin() + dest_data_offset);

                  /* Update SMA */
                  auto& sma_min = reinterpret_cast<int32_t&>(b->at(sma_pos));
                  auto& sma_max = reinterpret_cast<int32_t&>(b->at(sma_pos + sizeof(int32_t)));
                  if (sma_min > value || dest_cnt == 0) sma_min = value;
                  if (sma_max < value || dest_cnt == 0) sma_max = value;

                  pTupleOffsets[idx] = dest_data_offset;
                  record_size += sizeof(int32_t);

                }break;

                case ColumnInfo::Double_Type: {
                  /* Get Record Value */
                  uint16_t src_data_offset = data_pos + tupleOffset * sizeof(double);
                  const auto& value = reinterpret_cast<const double&>(b0->at(src_data_offset));

                  /* Insert Data */
                  uint16_t dest_data_offset = data_pos + dest_cnt * sizeof(double);
                  std::copy(reinterpret_cast<const uint8_t *>(&value),
                            reinterpret_cast<const uint8_t *>(&value) + sizeof(double),
                            b->begin() + dest_data_offset);

                  /* Update SMA */
                  auto& sma_min = reinterpret_cast<double&>(b->at(sma_pos));
                  auto& sma_max = reinterpret_cast<double&>(b->at(sma_pos + sizeof(double)));
                  if (sma_min > value || dest_cnt == 0) sma_min = value;
                  if (sma_max < value || dest_cnt == 0) sma_max = value;

                  pTupleOffsets[idx] = dest_data_offset;
                  record_size += sizeof(double);

                }break;

                case ColumnInfo::String_Type: {
                  /* Get Record Value */
                  const auto& value_offset = reinterpret_cast<const uint16_t&>(b0->at(data_pos + tupleOffset * nvm::gOffsetSize));
                  const auto& value = reinterpret_cast<const char (&)[]>(b0->at(value_offset));
                  std::size_t string_size = strlen(value) + 1;

                  /* Insert Data - Get target position */
                  uint16_t target_offset_pos = data_pos + dest_cnt * nvm::gOffsetSize;
                  uint16_t target_data_pos;
                  if (dest_cnt == 0) {
                    auto end_minipage = (idx < tInfo.numColumns() - 1) ?
                    reinterpret_cast<const uint16_t&>(b->at(nvm::gSmaOffsetPos + (idx + 1) * nvm::gAttrOffsetSize)) : nvm::gBlockSize;
                    target_data_pos = end_minipage - string_size;
                  } else /* cnt != 0 */{
                    const auto& last_offset = reinterpret_cast<const uint16_t&>(b->at(target_offset_pos - nvm::gOffsetSize));
                    target_data_pos = last_offset - string_size;
                  }

                  /* Insert Data - Set offset and string data */
                  std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                      reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::gOffsetSize,
                      b->begin() + target_offset_pos);
                  std::strcpy(reinterpret_cast<char(&)[]>(b->at(target_data_pos)), value);

                  /* Update SMA */
                  auto& sma_min_pos = reinterpret_cast<uint16_t &>(b->at(sma_pos));
                  auto& sma_max_pos = reinterpret_cast<uint16_t &>(b->at(sma_pos + nvm::gOffsetSize));
                  if (dest_cnt != 0) {
                    std::string sma_min(reinterpret_cast<const char (&)[]>(b->at(sma_min_pos)));
                    std::string sma_max(reinterpret_cast<const char (&)[]>(b->at(sma_max_pos)));
                    if (sma_min > value) sma_min_pos = target_data_pos;
                    else if (sma_max < value) sma_max_pos =target_data_pos;
                  } else /* cnt == 0 */{
                    sma_min_pos = target_data_pos;
                    sma_max_pos = target_data_pos;
                  }

                  /* Set new positions and sizes */
                  record_size += string_size + nvm::gOffsetSize;
                  pTupleOffsets[idx] = target_data_pos;

                }break;

                default: {
                  throw TableException("unsupported column type\n");
                }break;
              } /* switch (c.getType()) */
              ++idx;

            }
            /* Increase BDCC count */
            ++dest_cnt;
            /* Adapt Free Space */
            auto& fspace = reinterpret_cast<uint16_t&>(b->at(nvm::gFreeSpacePos));
            fspace -= record_size;
          }


          // adapt pointers
          if (root->block_list == nvmBlock) {
            root->block_list = newBlock1;
          } else {
            auto prevBlock = root->block_list;
            while (prevBlock->next != nvmBlock) {
              prevBlock = prevBlock->next;
            }
            prevBlock->next = newBlock1;
          }
          newBlock1->next = newBlock2;
          newBlock2->next = nvmBlock->next;
          nvmBlock->clear();
        });

    return std::make_pair(newBlock1, newBlock2); // blocks can be nullptr
  }


  std::bitset<32> getBDCCFromTuple (Tuple tp) {
    //TODO: Just use integers and shift operations instead of bitset
    const auto& bdccInfo = this->root->bdccInfo;
    const auto& tInfo = *this->root->tInfo;
    auto colCnt = 0; // total column counter
    auto dimCnt = 0; // dimension column counter
    auto bdccSize = 0; // sum of bits used for bdcc
    std::bitset<32> xtr = 0; // extracted bdcc value
    std::pair<int, std::bitset<32>> dimsBDCC[bdccInfo->bitMap.size()]; // (no. of bits, binned value)


    for (auto i = 0u; i < tInfo.numColumns(); i++) {
      if(bdccInfo->find(i) != bdccInfo->bitMap.cend()) {
      //if (bdccInfo->bitMap.find(i) != bdccInfo->bitMap.end()) {
        auto nBits = bdccInfo->find(i)->second; //bitMap.at(i);
        //get value
        auto value = ns_types::dynamic_get(colCnt, tp);
        //get bin
        int x = 0;
        if (value.type() == typeid(std::string)) {
          x = *reinterpret_cast<const int*>(boost::get<std::string>(value).c_str())
              & ((1L << nBits) - 1);
        } else if (value.type() == typeid(int)) {
          x = boost::get<int>(value) & ((1L << nBits) - 1);
        } else if (value.type() == typeid(double)) {
          x = static_cast<int>(boost::get<double>(value)) & ((1L << nBits) - 1);
        }
        dimsBDCC[dimCnt] = std::make_pair(nBits, std::bitset<32>(x));
        bdccSize += nBits;
        ++dimCnt;
      }
      ++colCnt;
    }

    /* Round robin the bins */
    while (bdccSize > 0) {
      for (auto k = 0u; k < bdccInfo->bitMap.size(); ++k) {
        if (dimsBDCC[k].first > 0) {
          xtr[bdccSize - 1] = dimsBDCC[k].second[dimsBDCC[k].first - 1];
          bdccSize--;
          dimsBDCC[k].first--;
        }
      }
    }
    return xtr;
  }
}; /* class persistent_table */

}} /* namespace pfabric::nvm */

#endif /* persistent_table_hpp_ */
