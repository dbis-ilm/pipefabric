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

#include <stddef.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include <core/PTuple.hpp>
#include <core/serialize.hpp>
#include <table/TableException.hpp>
#include <table/TableInfo.hpp>

#include <libpmemobj++/detail/persistent_ptr_base.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

using nvml::obj::pool;
using nvml::obj::pool_by_vptr;
using nvml::obj::pool_base;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::make_persistent;
using nvml::obj::delete_persistent;
using nvml::obj::transaction;

namespace pfabric { namespace nvm {

namespace detail {

template <typename Container>
inline std::string getStringFrom(const Container &container, size_t start_pos = 0)
{
  typename Container::const_iterator it_size;
  it_size = container.cbegin() + start_pos;
  auto it_data = it_size + sizeof(uint64_t);
  uint64_t string_size = *(uint64_t *)(&(*it_size));
  
  try {
    std::string value(string_size, '\0');
    for (size_t i = 0; i < string_size; ++i)
      value.at(i) = *(uint8_t *)(&(*it_data) + i);
  return value;
  } catch (std::bad_alloc& ba) {
    std::cerr << "bad_alloc caught. Could not create string for string size: "
              << string_size << " (" << ba.what() << ")\n";
  }
  return "";
}

} /* end namespace detail */

/**************************************************************************//**
 * \brief A persistent table used for PMEM technologies or emulations.
 *
 * \author Philipp Goetze <philipp.goetze@tu-ilmenau.de>
 *****************************************************************************/
template<class Tuple, typename K>
class persistent_table {

public:
  typedef persistent_ptr<Tuple> RecordType;
  typedef K KeyType; // BDCC Key?

  /************************************************************************//**
   * \brief Default Constructor.
   ***************************************************************************/
  persistent_table() {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {init(TableInfo());});
  }

  /************************************************************************//**
   * \brief Constructor for a given schema (TableInfo).
   ***************************************************************************/
  persistent_table(const TableInfo &_tInfo) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {init(_tInfo);});
  }

  /************************************************************************//**
   * \brief Inserts a new record into the persistent table to the fitting
   *        nvm_block.
   *
   * \param[in] rec
   *   the new record/tuple to insert.
   * \return 0 if successful
   ***************************************************************************/
  int insert(Tuple rec)
  {
    auto pop = pool_by_vptr(this);
    auto dest_block = root->block_list;
    auto b = dest_block->block;
    auto tInfo = *this->root->tInfo;
    auto record_size = 0;
    size_t idx = 0;
    size_t recOffset = 1;
    std::vector<uint16_t> pTupleOffsets;
    StreamType buf;
    StreamType &buf_ref = buf;
    rec.serializeToStream(buf);
    auto cnt = *(uint32_t *)(b->begin() + nvm::CountPos);
    auto space = *(uint16_t*)(b->begin() + nvm::FreeSpacePos);
    if (space < buf.size()) {
      std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
      throw TableException("Not enough space in block\n");
    }
    //TODO: Free Space check -> adapt Minipages or new Block
    

    transaction::exec_tx(pop, [&] {
      // Each attribute (SMA + Data) 
      for (auto &c : tInfo)
      {
        uint16_t sma_pos = *(uint16_t *)(b->begin() + nvm::SmaOffsetPos + idx * nvm::AttrOffsetSize);
        uint16_t data_pos = *(uint16_t *)(b->begin() + nvm::DataOffsetPos + idx * nvm::AttrOffsetSize);

        switch (c.mColType)
        {
        case ColumnInfo::Int_Type:
        {
          auto it_begin = buf_ref.cbegin() + recOffset;
          auto it_end = it_begin + sizeof(int32_t);
          int32_t value = deserialize<int32_t>(it_begin, it_end);
          recOffset += sizeof(int32_t);

          /* Check remaining space in minipage */
          uint16_t next_sma_pos;
          if (rec.size()==idx+1)
            next_sma_pos = nvm::BlockSize;
          else
            next_sma_pos = *(const uint16_t *)(b->begin() + nvm::SmaOffsetPos + (idx+1) * nvm::AttrOffsetSize);
          uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(int32_t));
          if(mp_free < sizeof(int32_t)) {
            std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
            transaction::abort(100);
          }

          /* Update SMA */
          int32_t sma_min = *(int32_t *)(b->begin() + sma_pos);
          int32_t sma_max = *(int32_t *)(b->begin() + sma_pos + sizeof(uint32_t));

          if (sma_min > value || cnt == 0) {
            std::copy(reinterpret_cast<const uint8_t *>(&value),
                      reinterpret_cast<const uint8_t *>(&value) + sizeof(uint32_t),
                      b->begin() + sma_pos);
          }
          if (sma_max < value || cnt == 0) {
            std::copy(reinterpret_cast<const uint8_t *>(&value),
                      reinterpret_cast<const uint8_t *>(&value) + sizeof(uint32_t),
                      b->begin() + (sma_pos + sizeof(uint32_t)));
          }

          /* Insert Data */
          uint16_t data_offset = data_pos + cnt * sizeof(uint32_t);
          std::copy(reinterpret_cast<const uint8_t *>(&value),
                    reinterpret_cast<const uint8_t *>(&value) + sizeof(uint32_t),
                    b->begin() + data_offset);
          record_size += sizeof(uint32_t);
          pTupleOffsets.push_back(data_offset);
        }
        break;
        case ColumnInfo::Double_Type:
        {
          auto it_begin = buf_ref.cbegin() + recOffset;
          auto it_end = it_begin + sizeof(double);
          double value = deserialize<double>(it_begin, it_end);
          recOffset += sizeof(double);

          /* Check remaining space in minipage */
          uint16_t next_sma_pos;
          if (rec.size()==idx+1)
            next_sma_pos = nvm::BlockSize;
          else
            next_sma_pos = *(const uint16_t *)(b->begin() + nvm::SmaOffsetPos + (idx+1) * nvm::AttrOffsetSize);
          uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(double));
          if(mp_free < sizeof(double)) {
            std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
            transaction::abort(100);
          }

          /* Update SMA */
          double sma_min = *(double *)(b->begin() + sma_pos);
          double sma_max = *(double *)(b->begin() + sma_pos + sizeof(uint64_t));

          if (sma_min > value || cnt == 0)
          {
            std::copy(reinterpret_cast<const uint8_t *>(&value),
                      reinterpret_cast<const uint8_t *>(&value) + sizeof(uint64_t),
                      b->begin() + sma_pos);
          }
          if (sma_max < value || cnt == 0)
          {
            std::copy(reinterpret_cast<const uint8_t *>(&value),
                      reinterpret_cast<const uint8_t *>(&value) + sizeof(uint64_t),
                      b->begin() + (sma_pos + sizeof(uint64_t)));
          }

          /* Insert Data */
          uint16_t data_offset = data_pos + cnt * sizeof(uint64_t);
          std::copy(reinterpret_cast<const uint8_t *>(&value),
                    reinterpret_cast<const uint8_t *>(&value) + sizeof(uint64_t),
                    b->begin() + data_offset);
          record_size += sizeof(uint64_t);
          pTupleOffsets.push_back(data_offset);
        }
        break;
        case ColumnInfo::String_Type:
        {
          auto it_begin = buf_ref.cbegin() + recOffset;
          auto value = detail::getStringFrom(buf_ref, recOffset);
          const char* c_value = value.c_str();
          uint64_t string_size = value.size() + 1;
          recOffset += string_size - 1 + sizeof(uint64_t);

          /* Check remaining space in minipage */
          uint16_t mp_free;
          if (cnt != 0) {
            uint16_t current_offset_pos = data_pos + cnt * nvm::OffsetSize ;
            uint16_t current_offset = *(uint16_t *)(b->begin() + current_offset_pos - nvm::OffsetSize);
            mp_free = current_offset - current_offset_pos;
          }
          else
          {
            uint16_t next_sma_pos;
            if (rec.size()==idx+1)
              next_sma_pos = nvm::BlockSize;
            else
              next_sma_pos = *(const uint16_t *)(b->begin() + nvm::SmaOffsetPos + (idx+1) * nvm::AttrOffsetSize);
            mp_free = next_sma_pos - data_pos;
          }

          if(mp_free < string_size + nvm::OffsetSize) {
            std::cerr << "Not enough space to insert tuple: (" << rec << ")" << std::endl;
            transaction::abort(100);
          }

          /* Insert Data - Get target position */
          uint16_t target_offset_pos = data_pos + cnt * nvm::OffsetSize;
          uint16_t target_data_pos;
          if (cnt == 0)
          {
            uint16_t end_minipage;
            if (idx < rec.size() - 1)
            {
              end_minipage = (*(uint16_t *)(b->begin() + nvm::SmaOffsetPos + (idx + 1) * nvm::AttrOffsetSize));
            }
            else
            {
              end_minipage = nvm::BlockSize;
            }
            target_data_pos = end_minipage - string_size;
          }
          else /* cnt != 0 */
          {
            uint16_t last_offset = *(uint16_t *)(b->begin() + target_offset_pos - nvm::OffsetSize);
            target_data_pos = last_offset - string_size;
          }

          /* Insert Data - Set offset and string data */
          std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                    reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::OffsetSize,
                    b->begin() + target_offset_pos);
          std::copy(reinterpret_cast<const uint8_t *>(c_value),
                  	reinterpret_cast<const uint8_t *>(c_value) + string_size,
                  	b->begin() + target_data_pos);

          /* Update SMA */
          if (cnt != 0)
          {
            uint16_t& sma_min_pos = reinterpret_cast<uint16_t &>(b->at(sma_pos));
            uint16_t& sma_max_pos = reinterpret_cast<uint16_t &>(b->at(sma_pos + nvm::OffsetSize));
            std::string sma_min(reinterpret_cast<const char (&)[]>(b->at(sma_min_pos)));
            std::string sma_max(reinterpret_cast<const char (&)[]>(b->at(sma_max_pos)));

            if (sma_min > value)
            {
              std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                        reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::OffsetSize,
                        b->begin() + sma_pos);
            }
            else if (sma_max < value)
            {
              std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                        reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::OffsetSize,
                        b->begin() + (sma_pos + nvm::OffsetSize));
            }
          }
          else /* cnt == 0 */
          {
            std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                      reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::OffsetSize,
                      b->begin() + sma_pos);
            std::copy(reinterpret_cast<const uint8_t *>(&target_data_pos),
                      reinterpret_cast<const uint8_t *>(&target_data_pos) + nvm::OffsetSize,
                      b->begin() + (sma_pos + nvm::OffsetSize));
          }

          record_size += string_size + nvm::OffsetSize;
          pTupleOffsets.push_back(target_data_pos);
        }
        break;
        default:
        {
          throw TableException("unsupported column type\n");
        }
        break;
        }
        ++idx;
      } /* for loop over Columns */

      /* Increase BDCC count */
      ++((uint32_t &)b->at(nvm::CountPos));
      /* Adapt Free Space */
      auto &fspace = ((uint16_t &)b->at(nvm::FreeSpacePos));
      fspace -= record_size;
    }); /* end of transaction */


    nvm::PTuple<decltype(rec)> ptp(b, pTupleOffsets);
    auto attr0 = get<3>(ptp);
    //std::cout << "PTuplePtr element at 0: " << attr0 << std::endl;

    /* TODO: Block across implementation (currently only one block supported)
    do
    {
      // Search correct block
      // Check for enough space
      //   yes, but not in minipage -> rearrange (average)
      //   no -> split block
      // Insert (For each attribute increase data vector and add value)
      // adapt SMAs and count  (How to cheaply increment)

      auto cur_block = dest_block->next;
      dest_block = cur_block;
    } while (dest_block != nullptr);
    */

    return 0;
  }

  /************************************************************************//**
   * \brief Prints the table content column-wise.
   *
   * \param[in] raw
   *   set to true to additionaly print out the complete raw byte arrays.
   ***************************************************************************/
  void print(bool raw = false)
  {

    auto dest_block = root->block_list;
    auto b = dest_block->block;
    auto tInfo = *this->root->tInfo;

    size_t colCnt = 0;
    for (auto &c : tInfo)
      colCnt++;

    auto key1 = *(uint32_t*)(b->begin());
    auto key2 = *(uint32_t*)(b->begin()+4);
    auto cnt = *(uint32_t*)(b->begin()+nvm::CountPos);
    auto space = *(uint16_t*)(b->begin()+nvm::FreeSpacePos);
    auto headerSize = nvm::FixedHeaderSize + nvm::AttrOffsetSize * colCnt;
    auto bodySize = nvm::BlockSize - headerSize;

    /* Plain byte-by-byte output */
    if (raw) 
    {
      size_t i = 0;
      printf("[ ");
      for (auto &byte : *b)
      {
        printf("%02x ", byte);
        if (++i % 32 == 0)
        {
          printf("]");
          if(i < b->size()) printf("\n[ ");
        }
      }
    }
    
    std::cout << "\nDDC Range min: " << key1  << '\n'
              << "DDC Range max: "   << key2  << '\n'
              << "Tuple count: "     << cnt   << '\n'
              << "Header size: "     << headerSize << " Bytes" << '\n'
              << "Body size: "       << bodySize   << " Bytes" << '\n'
              << "Free Space: "      << space      << " Bytes" << std::endl;

    if (cnt > 0)
    {
      size_t idx = 0;
      for (auto &c : tInfo)
      {
        uint16_t sma_pos = *(const uint16_t *)(b->begin() + nvm::SmaOffsetPos + idx * nvm::AttrOffsetSize);
        uint16_t data_pos = *(const uint16_t *)(b->begin() + nvm::DataOffsetPos + idx * nvm::AttrOffsetSize);

        switch (c.mColType)
        {
        case ColumnInfo::Int_Type:
        {
          auto &sma_min = (int32_t &)b->at(sma_pos);
          auto &sma_max = (int32_t &)b->at(sma_pos + sizeof(uint32_t));
          auto &data = (int32_t(&)[cnt])b->at(data_pos);

          /* Remaining Space */
          uint16_t next_sma_pos;
          if (colCnt==idx+1)
            next_sma_pos = nvm::BlockSize;
          else
            next_sma_pos = *(const uint16_t *)(b->begin() + nvm::SmaOffsetPos + (idx+1) * nvm::AttrOffsetSize);
          uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(int32_t));
         
          std::cout << "Column[" << idx << "]: " << c.mColName 
                    << "\n\tSpace left: "        << mp_free << " Bytes"
                    << "\n\tsma_min: "           << sma_min
                    << "\n\tsma_max: "           << sma_max
                    << "\n\tData: {";
          const char *padding = "";
          for (uint32_t i = 0; i < cnt; i++)
          {
            std::cout << padding << data[i];
            padding = ", ";
          }
          std::cout << "}\n" << std::endl;
        }
        break;
        case ColumnInfo::Double_Type:
        {
          auto &sma_min = (double &)b->at(sma_pos);
          auto &sma_max = (double &)b->at(sma_pos + sizeof(uint64_t));
          auto &data = (double(&)[cnt])b->at(data_pos);

          /* Remaining Space */
          uint16_t next_sma_pos;
          if (colCnt==idx+1)
            next_sma_pos = nvm::BlockSize;
          else
            next_sma_pos = *(const uint16_t *)(b->begin() + nvm::SmaOffsetPos + (idx+1) * nvm::AttrOffsetSize);
          uint16_t mp_free = next_sma_pos - data_pos - (cnt * sizeof(double));
          
          std::cout << "Column[" << idx << "]: " << c.mColName 
                    << "\n\tSpace left: "        << mp_free << " Bytes"
                    << "\n\tsma_min: "           << sma_min
                    << "\n\tsma_max: "           << sma_max
                    << "\n\tData: {";
          const char *padding = "";
          for (uint32_t i = 0; i < cnt; i++)
          {
            std::cout << padding << data[i];
            padding = ", ";
          }
          std::cout << "}\n" << std::endl;
        }
        break;
        case ColumnInfo::String_Type:
        {
          auto &sma_min_pos = (uint16_t &)b->at(sma_pos);
          auto &sma_max_pos = (uint16_t &)b->at(sma_pos + nvm::OffsetSize);
          auto &string_pos = (uint16_t(&)[cnt])b->at(data_pos);
          auto sma_min(reinterpret_cast<const char (&)[]>(b->at(sma_min_pos)));
          auto sma_max(reinterpret_cast<const char (&)[]>(b->at(sma_max_pos)));

          uint16_t current_offset_pos = data_pos + cnt * nvm::OffsetSize ;
          uint16_t current_offset = *(uint16_t *)(b->begin() + current_offset_pos - nvm::OffsetSize);
          uint16_t mp_free = current_offset - current_offset_pos;

          std::cout << "Column[" << idx << "]: " << c.mColName
                    << "\n\tSpace left: "        << mp_free << " Bytes"
                    << "\n\tsma_min: " << sma_min
                    << "\n\tsma_max: " << sma_max
                    << "\n\tData: {";
          const char *padding = "";
          for (uint32_t i = 0; i < cnt; i++)
          {
            std::string data(reinterpret_cast<char (&)[]>(b->at(string_pos[i])));
            std::cout << padding << data;
            padding = ", ";
          }
          std::cout << "}\n" << std::endl;
        }
        break;
        default:
        {
          throw TableException("unsupported column type\n");
        }
        break;
        }
        ++idx;
      } /* end for (auto &c: tInfo) */
    } /* if cnt > 0 */

    
  }

  // TODO: For later
  int update(RecordType rec, KeyType key)
  {
    return 0;
  }

  int deleteByKey(KeyType key)
  {
    return 0;
  }

  RecordType getByKey(KeyType key)
  {
  }

// Private ////////////////////////////////////////////////////////////////////
private:
  struct nvm_block
  {
    nvm_block() : next(nullptr), block(nullptr) {}
    nvm_block(nvm::NVM_Block _block) : next(nullptr), block(_block) {}

    persistent_ptr<struct nvm_block> next;
    persistent_ptr<nvm::NVM_Block> block;

    void clear()
    {
      if (next)
      {
        delete_persistent<struct nvm_block>(next);
        next = nullptr;
      }
      if (block)
      {
        delete_persistent<nvm::NVM_Block>(block);
        block = nullptr;
      }
    }
  };

  struct root
  {
    persistent_ptr<struct nvm_block> block_list;
    persistent_ptr<const TableInfo> tInfo;
    //persistent_ptr<std::string> tName;
  };

  persistent_ptr<struct root> root;
  
  struct ColumnInfoCompare
  {
    bool operator() (const ColumnInfo& lhs, const ColumnInfo& rhs) const
    {
      return lhs.mColName < rhs.mColName;
    }
  };
  typedef std::map<ColumnInfo, uint16_t, ColumnInfoCompare> ColumnIntMap;
  ColumnIntMap calcMinipageSizes(const TableInfo& tableInfo, uint16_t totalSize, ColumnIntMap customizations = ColumnIntMap()) 
  {
    size_t portions = 0;
    ColumnIntMap mp_sizes = ColumnIntMap();
    for (auto &c: tableInfo) {
      if (customizations.find(c) == customizations.end())
      {
        switch (c.mColType)
        {
          case ColumnInfo::Int_Type:    portions += 1; break;
          case ColumnInfo::Double_Type: portions += 2; break;
          case ColumnInfo::String_Type: portions += 5; break;
          default: throw TableException("unsupported column type\n"); break;
        }
      } 
      else
        portions += customizations[c];
    }

    for (auto &c: tableInfo) {
      if (customizations.find(c) == customizations.end())
      {
        switch (c.mColType)
        {
          case ColumnInfo::Int_Type:    mp_sizes[c] = 1 * totalSize / portions; break;
          case ColumnInfo::Double_Type: mp_sizes[c] = 2 * totalSize / portions; break;
          case ColumnInfo::String_Type: mp_sizes[c] = 5 * totalSize / portions; break;
          default: throw TableException("unsupported column type\n"); break;
        }
      } 
      else
        mp_sizes[c] = customizations[c] * totalSize / portions;
    }
    return mp_sizes;
  }


  /************************************************************************//**
   * \brief Initialization function for creating the necessary structures.
   *
   * \param[in] _tInfo
   *   the underlying schema to use
   ***************************************************************************/
  void init(const TableInfo &_tInfo) {
    this->root = make_persistent<struct root>();
    this->root->tInfo = make_persistent<TableInfo>(_tInfo);
    this->root->block_list = make_persistent<struct nvm_block>();
    nvm::NVM_Block first = initBlock();
    this->root->block_list->block = make_persistent<nvm::NVM_Block>(first);
  }

  /************************************************************************//**
   * \brief Initialize a new nvm::NVM_Block.
   *
   * \return a new initialized nvm::NVM_Block.
   ***************************************************************************/
  nvm::NVM_Block initBlock()
  {
    auto b = nvm::NVM_Block{
        0x00, 0x00, 0x00, 0x00,  // BDCC Range
        0xFF, 0xFF, 0xFF, 0xFF,  // BDCC Range
        0x00, 0x00, 0x00, 0x00}; // Tuple Count

    auto colCnt = 0;
    for (auto &c : *this->root->tInfo)
      colCnt++;

    uint16_t header_size = nvm::FixedHeaderSize + colCnt * nvm::AttrOffsetSize;
    uint16_t body_size = nvm::BlockSize - header_size;
    auto minipage_size = body_size / colCnt;

    auto sizes = calcMinipageSizes(*this->root->tInfo, body_size);

    /* Set Offsets */
    size_t idx = 0;
    size_t sma_size = 0;
    uint16_t current_offset = header_size;
    for (auto &c : *this->root->tInfo)
    {
      uint16_t sma_offset;
      uint16_t data_offset;
      switch (c.mColType)
      {
      case ColumnInfo::Int_Type:
      {
        sma_offset = current_offset;
        data_offset = current_offset + 2 * sizeof(uint32_t);
        sma_size += 2 * sizeof(uint32_t);
        current_offset += sizes[c];
      }
      break;
      case ColumnInfo::Double_Type:
      {
        sma_offset = current_offset;
        data_offset = current_offset + 2 * sizeof(uint64_t);
        sma_size += 2 * sizeof(uint64_t);
        current_offset += sizes[c];
      }
      break;
      case ColumnInfo::String_Type:
      {
        sma_offset = current_offset;
        data_offset = current_offset + nvm::AttrOffsetSize;
        sma_size += nvm::AttrOffsetSize;
        current_offset += sizes[c];
      }
      break;
      default:
      {
        throw TableException("unsupported column type\n");
      }
      break;
      }

      std::copy(reinterpret_cast<const uint8_t *>(&sma_offset),
                reinterpret_cast<const uint8_t *>(&sma_offset) + nvm::OffsetSize,
                b.begin() + (nvm::SmaOffsetPos + idx * nvm::AttrOffsetSize));
      std::copy(reinterpret_cast<const uint8_t *>(&data_offset),
                reinterpret_cast<const uint8_t *>(&data_offset) + nvm::OffsetSize,
                b.begin() + (nvm::DataOffsetPos + idx * nvm::AttrOffsetSize));
      ++idx;
    }

    /* Set Free Space field */
    uint16_t free_space = body_size - sma_size;
    std::copy(reinterpret_cast<const uint8_t *>(&free_space),
              reinterpret_cast<const uint8_t *>(&free_space) + sizeof(uint16_t),
              b.begin() + nvm::FreeSpacePos);
    
    return b;
  }

  void insert_block()
  {
  }
}; /* class persistent_table */

} /* namespace nvm */

} /* namespace pfabric */

#endif /* persistent_table_hpp_ */
