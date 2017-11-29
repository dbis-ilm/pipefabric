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

#ifndef PTable_hpp_
#define PTable_hpp_

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
#include <execinfo.h>
#include "boost/variant.hpp"

#include "core/serialize.hpp"
#include "nvm/BDCCInfo.hpp"
#include "nvm/PBPTree.hpp"
#include "nvm/PTableInfo.hpp"
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

namespace pfabric {
namespace nvm {

namespace detail {

template<typename T>
inline void copyToByteArray(BDCC_Block &b, const T &data, const std::size_t size, const std::size_t targetPos) {
  std::copy(reinterpret_cast<const uint8_t *>(&data),
            reinterpret_cast<const uint8_t *>(&data) + size,
            b.begin() + targetPos);
}

static void mybacktrace() {
  int j, nptrs;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);
  printf("backtrace() returned %d addresses\n", nptrs);

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (j = 0; j < nptrs; j++)
    printf("%s\n", strings[j]);

  free(strings);
}
} /* end of namespace detail*/

using nvml::obj::pool;
using nvml::obj::pool_by_vptr;
using nvml::obj::pool_base;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::make_persistent;
using nvml::obj::delete_persistent;
using nvml::obj::transaction;

const std::string LAYOUT = "PTable";
const auto BRANCHKEYS = 144;
const auto LEAFKEYS = 144; //TODO: Should be calculated based on Tuple size, 1168 (32KB)
const auto NOT_ENGOUGH_SPACE = 1;

using ColumnIntMap = std::map<uint16_t, uint16_t>;
using IntDoubleString = boost::variant<int, double, std::string>;
using ColumnRangeMap = std::unordered_map<uint16_t, std::pair<IntDoubleString , IntDoubleString>>;

/**************************************************************************//**
 * \brief A persistent table used for PMEM technologies or emulations.
 *
 * \author Philipp Goetze <philipp.goetze@tu-ilmenau.de>
 *****************************************************************************/
template<class Tuple, typename KeyType>
class PTable {
  using IndexType = PBPTree<KeyType, PTuple<Tuple, KeyType>, BRANCHKEYS, LEAFKEYS>;
  using DataNodePtr = persistent_ptr<struct DataNode<KeyType>>;

  /************************************************************************//**
   * \brief Iterator to iterate over all tuples using the blocks.
   ***************************************************************************/
  class BlockIterator {
    using DataNodeVector = std::vector<persistent_ptr<DataNode<KeyType>>>;
    const PTable<Tuple, KeyType> parent;
    const ColumnRangeMap predicates;
    const DataNodeVector candidates;
    typename DataNodeVector::const_iterator currentNode;
    uint16_t currentCnt = 0u;
    uint16_t currentPos = 0u;

   public:
    BlockIterator(const PTable<Tuple, KeyType> &_parent, const ColumnRangeMap &_predicates) :
      parent(_parent),
      predicates(_predicates),
      candidates(parent.getCandidateBlocks(predicates)),
      currentNode(candidates.cbegin()),
      currentCnt(candidates.size() > 0 ? reinterpret_cast<const uint32_t &>((*currentNode)->block.get_ro()[gCountPos]) : 0) {
      if (candidates.size() > 0) ++(*this);
      else currentPos = 1; // --> setting to end()
    }

    BlockIterator(const PTable<Tuple, KeyType> &_parent,
                  const ColumnRangeMap &_predicates,
                  const DataNodeVector &_candidates) :
      parent(_parent),
      predicates(_predicates),
      candidates(_candidates),
      currentNode(candidates.cbegin()),
      currentCnt(candidates.size() > 0 ? reinterpret_cast<const uint32_t &>((*currentNode)->block.get_ro()[gCountPos]) : 0) {
      if (candidates.size() > 0) ++(*this);
      else currentPos = 1; // --> setting to end()
    }

    BlockIterator(const PTable<Tuple, KeyType> &_parent) :
      parent(_parent), currentNode(nullptr), currentPos(1) {}

    BlockIterator &operator++() {
      if (++currentPos > currentCnt) {
        ++currentNode;
        currentPos = 1u;
        if (*currentNode != nullptr) {
          currentCnt = reinterpret_cast<const uint32_t &>((*currentNode)->block.get_ro()[gCountPos]);
        } else {
          currentCnt = 0u;
          return *this;
        }
      }

      // Check Tuple for inRange
      if (parent.isPTupleinRange(**this, predicates)){
        return *this;
      } else {
        return ++(*this);
      }
    }

    BlockIterator &next() {
      return ++(*this);
    }

    BlockIterator operator++(int) {
      BlockIterator retval = *this;
      ++(*this);
      return retval;
    }

    BlockIterator begin() {
      return BlockIterator(parent, predicates, candidates);
    }

    BlockIterator end() {
      return BlockIterator(parent);
    }

    bool operator==(BlockIterator other) const {
      return (
//        currentBlock == other.currentBlock &&  // This prevents the comparison with end()
        currentPos == other.currentPos &&
        currentCnt == other.currentCnt);
    }
    bool operator!=(BlockIterator other) const { return !(*this == other); }

    PTuple<Tuple, KeyType> operator*() {
      std::array<uint16_t, Tuple::NUM_ATTRIBUTES> pTupleOffsets;
      auto idx = 0u;
      for (const auto &c : *parent.root->tInfo) {
        const auto
          &dataPos = reinterpret_cast<const uint16_t &>((*currentNode)->block.get_ro()[gDataOffsetPos + idx * gAttrOffsetSize]);
        uint16_t dataOffset;
        switch (c.getType()) {
          case ColumnInfo::Int_Type: {
            dataOffset = dataPos + (currentPos-1) * sizeof(int32_t);
          }
            break;
          case ColumnInfo::Double_Type: {
            dataOffset = dataPos + (currentPos-1) * sizeof(double);
          }
            break;
          case ColumnInfo::String_Type: {
            dataOffset = reinterpret_cast<const uint16_t &>((*currentNode)->block.get_ro()[dataPos + (currentPos-1) * gOffsetSize]);
          }
            break;
          default: throw TableException("unsupported column type\n");
        }
        pTupleOffsets[idx] = dataOffset;
        ++idx;
      }
      return *(new PTuple<Tuple, KeyType>(*currentNode, pTupleOffsets));
    }

    // iterator traits
    using difference_type = long;
    using value_type = PTuple<Tuple, KeyType>;
    using pointer = const PTuple<Tuple, KeyType> *;
    using reference = const PTuple<Tuple, KeyType> &;
    using iterator_category = std::forward_iterator_tag;
  };

 public:
  /************************************************************************//**
   * \brief Public Iterator to iterate over all inserted tuples using the index.
   ***************************************************************************/
  class iterator {
   public:
    using Predicate = std::function<bool(const nvm::PTuple<Tuple, KeyType> &)>;
    using TreeIter = typename IndexType::iterator;

    iterator() : treeIter() {}

    iterator(TreeIter _iter, TreeIter _end, Predicate _pred = [](const nvm::PTuple<Tuple, KeyType> &) { return true; })
      : treeIter(_iter), end(_end), pred(_pred) {
      while (isValid() && !pred((*treeIter).second))
        treeIter++;
    }

    iterator &operator++() {
      treeIter++;
      while (isValid() && !pred((*treeIter).second))
        treeIter++;
      return *this;
    }

    iterator &next() {
      treeIter++;
      while (isValid() && !pred((*treeIter).second))
        treeIter++;
      return *this;
    }

    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }

    inline const bool isValid() const {
      return treeIter != end;
    }

    bool operator==(iterator other) const { return (treeIter == other.treeIter); }

    bool operator!=(iterator other) const { return !(treeIter == other.treeIter); }

    PTuple<Tuple, KeyType> operator*() {
      return (*treeIter).second;
    }

    // iterator traits
    using difference_type = long;
    using value_type = PTuple<Tuple, KeyType>;
    using pointer = const PTuple<Tuple, KeyType> *;
    using reference = const PTuple<Tuple, KeyType> &;
    using iterator_category = std::forward_iterator_tag;
   protected:
    TreeIter treeIter, end;
    Predicate pred;
  };

  iterator begin() { return iterator(root->index->begin(), root->index->end()); }

  iterator end() { return iterator(); }



  /************************************************************************//**
   * \brief Default Constructor.
   ***************************************************************************/
  PTable() {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] { init("", {}, ColumnIntMap()); });
  }

  /************************************************************************//**
   * \brief Constructor for a given schema and dimension clustering.
   ***************************************************************************/
  PTable(const std::string &tName, ColumnInitList columns,
         const ColumnIntMap &_bdccInfo = ColumnIntMap()) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] { init(tName, columns, _bdccInfo); });
  }

  /************************************************************************//**
   * \brief Constructor for a given schema (using TableInfo) and dimension clustering.
   ***************************************************************************/
  PTable(const TableInfo &tInfo, const ColumnIntMap &_bdccInfo = ColumnIntMap()) {
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] { init(tInfo, _bdccInfo); });
  }

  /************************************************************************//**
   * \brief Default Destructor.
   ***************************************************************************/
  ~PTable() {
//    PLOG("closing pool");
//    auto pop = pool_by_vptr(this);
//    pop.close();
//    PLOG("closed");
  }

  iterator select(typename iterator::Predicate func) {
    return iterator(root->index->begin(), root->index->end(), func);
  }

  /************************************************************************//**
   * \brief Inserts a new record into the persistent table to the fitting
   *        DataNode.
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
     *   X no -> split block
     * X Insert + adapt SMAs and count
     */

    //TODO: Check if already inserted in Index (Update or Exception?)
    auto targetNode = root->dataNodes;
    StreamType buf;
    rec.serializeToStream(buf);

    /* Calculate BDCC value for input tuple*/
    auto xtr = static_cast<uint32_t>(getBDCCFromTuple(rec).to_ulong());
    /* Search for correct Block */
    do {
      /* Retrieve DDC Range */
      const auto& ddc_min = reinterpret_cast<const uint32_t &>(targetNode->block.get_ro()[gDDCRangePos1]);
      const auto& ddc_max = reinterpret_cast<const uint32_t &>(targetNode->block.get_ro()[gDDCRangePos2]);

      if (xtr >= ddc_min && xtr <= ddc_max) break; //Found correct block

      targetNode = targetNode->next;
    } while (targetNode != nullptr); /* Should not reach end! */

    auto needsSplit = findInsertNodeOrSplit(targetNode, rec);
    if (needsSplit) {
      PLOG("Need to split for tuple: " << rec);
      try {
        auto pop = pool_by_vptr(this);
        std::pair<DataNodePtr, DataNodePtr> newNodes;
        if (pmemobj_tx_stage() == TX_STAGE_NONE)
          transaction::exec_tx(pop, [&] { newNodes = splitBlock(targetNode); });
        else
          newNodes = splitBlock(targetNode);
        return insert(key, rec);
      } catch (std::exception &te) {
        std::cerr << te.what() << '\n'
                  << "Splitting table block failed. Tuple not inserted: "
                  << rec << '\n';
        detail::mybacktrace();
      }
    }

    return insertTuple(key, rec, targetNode);
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
  int updateAttribute(KeyType key, size_t pos, Tuple rec) {
    //TODO: Implement. But is this really necessary?
    throw TableException("Updating a single attribute is not implemented, yet.");
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
  int updateComplete(KeyType key, Tuple rec) {
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
    std::size_t nres;
    auto pop = pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      //TODO: Also delete data?
      //delete_persistent<PTuple<Tuple>>(getByKey(key));
      nres = static_cast<std::size_t>(root->index->erase(key));
     //TODO: Delete from Historgram
//      const auto &ptp = getByKey(key);
//      const auto xtr = static_cast<uint32_t>(getBDCCFromTuple(*ptp.createTuple()).to_ulong());

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
  PTuple<Tuple, KeyType> getByKey(KeyType key) const {
    PTuple<Tuple, KeyType> val;
    if (root->index->lookup(key, &val)) {
      return val;
    } else {
      throw TableException("key not found");
    }
  }

  /************************************************************************//**
   * \brief Return an iterator to navigate over all tuples being valid
   *        regarding the \p rangePredicates.
   *
   *
   * \param[in] rangePredicates
   *   the range predicates used for this scan represented as map using a
   *   column id to a min and max value.
   * \return
   *   a \c BlockIterator for navigating through the valid tuples.
   ***************************************************************************/
  BlockIterator rangeScan(const ColumnRangeMap &rangePredicates) const {
    return BlockIterator(*this, rangePredicates);
  }

  /************************************************************************//**
   * @brief Return the number of tuples stored in the table.
   *
   * @return the number of tuples
   ***************************************************************************/
  unsigned long count() const {
    auto cnt = 0ul;
    auto targetNode = root->dataNodes;
    do {
      cnt += reinterpret_cast<const uint32_t &>(targetNode->block.get_ro()[gCountPos]);
      targetNode = targetNode->next;
    } while (targetNode != nullptr);
    return cnt;
  }

  /************************************************************************//**
   * \brief Prints the table content column-wise.
   *
   * \param[in] raw
   *   set to true to additionaly print out the complete raw byte arrays.
   ***************************************************************************/
  void print(bool raw = false) {

    auto currentNode = root->dataNodes;
    const auto &tInfo = *root->tInfo;
    const auto colCnt = tInfo.numColumns();

    do {
      auto b = currentNode->block.get_ro();

      const auto &key1 = reinterpret_cast<const uint32_t &>(b[gDDCRangePos1]);
      const auto &key2 = reinterpret_cast<const uint32_t &>(b[gDDCRangePos2]);
      const auto &cnt = reinterpret_cast<const uint32_t &>(b[gCountPos]);
      const auto &space = reinterpret_cast<const uint16_t &>(b[gFreeSpacePos]);
      const auto headerSize = gFixedHeaderSize + gAttrOffsetSize * colCnt;
      const auto bodySize = gBlockSize - headerSize;

      /* Plain byte-by-byte output */
      if (raw) {
        size_t i = 0;
        printf("[ ");
        for (auto &byte : b) {
          printf("%02x ", byte);
          if (++i % 32 == 0) {
            printf("]");
            if (i < b.size())
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
        for (const auto &c : tInfo) {
          std::cout << "Column Info: " << c.getName() << ": " << c.getType() << std::endl;
          const auto &smaPos = reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos + idx * gAttrOffsetSize]);
          const auto &dataPos = reinterpret_cast<const uint16_t &>(b[gDataOffsetPos + idx * gAttrOffsetSize]);

          switch (c.getType()) {
            case ColumnInfo::Int_Type: {
              const auto &smaMin = reinterpret_cast<const int32_t &>(b[smaPos]);
              const auto &smaMax = reinterpret_cast<const int32_t &>(b[smaPos + sizeof(int32_t)]);
              const auto &data = reinterpret_cast<const int32_t(&)[cnt]>(b[dataPos]);

              /* Remaining Space */
              auto nextSmaPos = (colCnt == idx + 1) ?
                                gBlockSize :
                                reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos + (idx + 1) * gAttrOffsetSize]);
              const auto freeSpaceMiniPage = nextSmaPos - dataPos - (cnt * sizeof(int32_t));

              std::cout << "Column[" << idx << "]: " << c.getName()
                        << "\n\tSpace left: " << freeSpaceMiniPage << " Bytes"
                        << "\n\tsmaMin: " << smaMin
                        << "\n\tsmaMax: " << smaMax
                        << "\n\tData: {";
//              const char *padding = "";
//              for (auto i = 0u; i < cnt; i++) {
//                std::cout << padding << data[i];
//                padding = ", ";
//              }
              std::cout << "}\n";
            }
              break;

            case ColumnInfo::Double_Type: {
              const auto &smaMin = reinterpret_cast<const double &>(b[smaPos]);
              const auto &smaMax = reinterpret_cast<const double &>(b[smaPos + sizeof(double)]);
              const auto &data = reinterpret_cast<const double (&)[cnt]>(b[dataPos]);

              /* Remaining Space */
              auto nextSmaPos = (colCnt == idx + 1) ?
                                gBlockSize :
                                reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos + (idx + 1) * gAttrOffsetSize]);
              const auto freeSpaceMiniPage = nextSmaPos - dataPos - (cnt * sizeof(double));

              std::cout << "Column[" << idx << "]: " << c.getName()
                        << "\n\tSpace left: " << freeSpaceMiniPage << " Bytes"
                        << "\n\tsmaMin: " << smaMin
                        << "\n\tsmaMax: " << smaMax
                        << "\n\tData: {";
//              const char *padding = "";
//              for (auto i = 0u; i < cnt; i++) {
//                std::cout << padding << data[i];
//                padding = ", ";
//              }
              std::cout << "}\n";
            }
              break;

            case ColumnInfo::String_Type: {
              auto &smaMinPos = (uint16_t &) b[smaPos];
              auto &smaMaxPos = (uint16_t &) b[smaPos + gOffsetSize];
              auto &stringPos = (uint16_t (&)[cnt]) b[dataPos];
              auto smaMin(reinterpret_cast<const char (&)[]>(b[smaMinPos]));
              auto smaMax(reinterpret_cast<const char (&)[]>(b[smaMaxPos]));

              auto currentOffsetPos = dataPos + cnt * gOffsetSize;
              auto currentOffset = reinterpret_cast<const uint16_t &>(b[currentOffsetPos - gOffsetSize]);
              auto freeSpaceMiniPage = currentOffset - currentOffsetPos;

              std::cout << "Column[" << idx << "]: " << c.getName()
                        << "\n\tSpace left: " << freeSpaceMiniPage << " Bytes"
                        << "\n\tsmaMin: " << smaMin
                        << "\n\tsmaMax: " << smaMax
                        << "\n\tData: {";
//              const char *padding = "";
//              for (auto i = 0u; i < cnt; i++) {
//                std::string data(reinterpret_cast<char (&)[]>(b[stringPos[i]]));
//                std::cout << padding << data;
//                padding = ", ";
//              }
              std::cout << "}\n";
            }
              break;

            default:throw TableException("unsupported column type\n");
          }
          ++idx;
        } /* end for (auto &c: tInfo) */
      } /* if cnt > 0 */
      std::cout << "\n";
      currentNode = currentNode->next;
    } while (currentNode != nullptr);
  }

// Private ////////////////////////////////////////////////////////////////////
 private:

  struct root {
    DataNodePtr dataNodes;
    persistent_ptr<IndexType> index;
    persistent_ptr<PTableInfo> tInfo;
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
    auto portions = 0u;
    ColumnIntMap miniPageSizes = ColumnIntMap();
    /* Get the sum of all column portitions */
    for (auto i = 0u; i < root->tInfo->numColumns(); i++) {
      const auto &c = root->tInfo->columnInfo(i);
      if (customizations.find(i) == customizations.end()) {
        switch (c.getType()) {
          case ColumnInfo::Int_Type:portions += 1;
            break;
          case ColumnInfo::Double_Type:portions += 2;
            break;
          case ColumnInfo::String_Type:portions += 5;
            break;
          default:throw TableException("unsupported column type\n");
        }
      } else
        portions += customizations[i];
    }
    /* Calculate and save the minipage sizes for all columns */
    for (auto i = 0u; i < root->tInfo->numColumns(); i++) {
      const auto &c = root->tInfo->columnInfo(i);
      if (customizations.find(i) == customizations.end()) {
        switch (c.getType()) {
          case ColumnInfo::Int_Type:miniPageSizes[i] = 1 * totalSize / portions;
            break;
          case ColumnInfo::Double_Type:miniPageSizes[i] = 2 * totalSize / portions;
            break;
          case ColumnInfo::String_Type:miniPageSizes[i] = 5 * totalSize / portions;
            break;
          default:throw TableException("unsupported column type\n");
        }
      } else
        miniPageSizes[i] = customizations[i] * totalSize / portions;
    }
    return miniPageSizes;
  }

  /************************************************************************//**
   * \brief Initialization function for creating the necessary structures.
   *
   * \param[in] _tName
   *   the name of the table
   * \param[in] _columns
   *   a list of column name and type pairs
   * \param[in] _bdccInfo
   *   a mapping of column ids to number of BDCC bits to use
   ***************************************************************************/
  void init(const std::string &_tName, const ColumnInitList &_columns, const ColumnIntMap &_bdccInfo) {
    this->root = make_persistent<struct root>();
    this->root->tInfo.get_rw() = make_persistent<PTableInfo>(_tName, _columns);
    this->root->bdccInfo = make_persistent<BDCCInfo>(_bdccInfo);
    this->root->index = make_persistent<IndexType>();
    this->root->dataNodes = make_persistent<struct DataNode<KeyType>>();
    this->root->dataNodes->block.get_rw() = initBlock(0, ((1L << this->root->bdccInfo->numBins()) - 1));
  }

  /************************************************************************//**
   * \brief Initialization function for creating the necessary structures.
   *
   * \param[in] _tInfo
   *   the underlying schema to use
   * \param[in] _bdccInfo
   *   a mapping of column ids to number of BDCC bits to use
   ***************************************************************************/
  void init(const TableInfo &_tInfo, const ColumnIntMap &_bdccInfo) {
    this->root = make_persistent<struct root>();
    this->root->tInfo = make_persistent<PTableInfo>(_tInfo);
    this->root->bdccInfo = make_persistent<BDCCInfo>(_bdccInfo);
    this->root->index = make_persistent<IndexType>();
    this->root->dataNodes = make_persistent<struct DataNode<KeyType>>();
    this->root->dataNodes->block.get_rw() = initBlock(0, ((1L << this->root->bdccInfo->numBins()) - 1));
  }

  /************************************************************************//**
   * \brief Initialize a new BDCC_Block.
   *
   * \return a new initialized BDCC_Block.
   ***************************************************************************/
  BDCC_Block initBlock(const uint32_t &ddc0, const uint32_t &ddc1) {
    const auto &tInfo = *root->tInfo;
    auto b = BDCC_Block{};

    /* Set DDC Range */
    detail::copyToByteArray(b, ddc0, gDDCValueSize, gDDCRangePos1);
    detail::copyToByteArray(b, ddc1, gDDCValueSize, gDDCRangePos2);

    const auto colCnt = tInfo.numColumns();
    const auto headerSize = gFixedHeaderSize + colCnt * gAttrOffsetSize;
    const auto bodySize = gBlockSize - headerSize;
    const auto miniPageSizes = calcMinipageSizes(bodySize);

    /* Set Offsets */
    auto idx = 0u;
    auto smaSize = 0u;
    auto currentOffset = std::move(headerSize);
    for (auto i = 0u; i < colCnt; i++) {
      const auto &c = tInfo.columnInfo(i);
      uint16_t smaOffset = currentOffset;
      uint16_t dataOffset;
      switch (c.getType()) {
        case ColumnInfo::Int_Type: {
          dataOffset = smaOffset + 2 * sizeof(uint32_t);
          smaSize += 2 * sizeof(uint32_t);
        }
          break;
        case ColumnInfo::Double_Type: {
          dataOffset = smaOffset + 2 * sizeof(uint64_t);
          smaSize += 2 * sizeof(uint64_t);
        }
          break;
        case ColumnInfo::String_Type: {
          dataOffset = smaOffset + gAttrOffsetSize;
          smaSize += gAttrOffsetSize;
        }
          break;
        default:throw TableException("unsupported column type\n");
      }
      currentOffset += miniPageSizes.at(i);

      /* Set/Save SMA and data offset for this attribute */
      detail::copyToByteArray(b, smaOffset, gOffsetSize, gSmaOffsetPos + idx * gAttrOffsetSize);
      detail::copyToByteArray(b, dataOffset, gOffsetSize, gDataOffsetPos + idx * gAttrOffsetSize);

      ++idx;
    }

    /* Set Free Space field */
    const uint16_t freeSpace = bodySize - smaSize;
    detail::copyToByteArray(b, freeSpace, gOffsetSize, gFreeSpacePos);

    return b;
  }

  /************************************************************************//**
   * \brief Insert a new Tuple into the given block.
   *
   * \return number of inserted elements.
   ***************************************************************************/
  int insertTuple(KeyType key, Tuple tp, DataNodePtr &targetNode) {
    auto pop = pool_by_vptr(this);
    auto &b = targetNode->block.get_rw();
    const auto &tInfo = *this->root->tInfo;
    auto recordSize = 0u;
    auto recordOffset = 1u;
    auto idx = 0u;
    auto &cnt = reinterpret_cast<uint32_t &>(b[gCountPos]);
    auto &freeSpace = reinterpret_cast<uint16_t &>(b[gFreeSpacePos]);
    std::array<uint16_t, Tuple::NUM_ATTRIBUTES> pTupleOffsets;
    StreamType buf;
    tp.serializeToStream(buf);

    try {
      transaction::exec_tx(pop, [&] {
        // Each attribute (SMA + Data)
        for (auto &c : tInfo) {
          const auto &smaPos = reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos + idx * gAttrOffsetSize]);
          const auto &dataPos = reinterpret_cast<const uint16_t &>(b[gDataOffsetPos + idx * gAttrOffsetSize]);

          switch (c.getType()) {

            case ColumnInfo::Int_Type: {
              /* Get Record Value */
              auto iterBegin = buf.cbegin() + recordOffset;
              auto iterEnd = iterBegin + sizeof(int32_t);
              const auto value = deserialize<int32_t>(iterBegin, iterEnd);

              /* Insert Data */
              const auto dataOffset = dataPos + cnt * sizeof(int32_t);
              detail::copyToByteArray(b, value, sizeof(int32_t), dataOffset);

              /* Update SMA */
              auto &smaMin = reinterpret_cast<int32_t &>(b[smaPos]);
              auto &smaMax = reinterpret_cast<int32_t &>(b[smaPos + sizeof(int32_t)]);
              if (smaMin > value || cnt == 0) smaMin = value;
              if (smaMax < value || cnt == 0) smaMax = value;

              /* Set new positions and sizes */
              recordOffset += sizeof(int32_t);
              recordSize += sizeof(int32_t);
              pTupleOffsets[idx] = dataOffset;

            }
              break;

            case ColumnInfo::Double_Type: {
              /* Get Record Value */
              auto iterBegin = buf.cbegin() + recordOffset;
              auto iterEnd = iterBegin + sizeof(double);
              const auto value = deserialize<double>(iterBegin, iterEnd);

              /* Insert Data */
              const auto dataOffset = dataPos + cnt * sizeof(double);
              detail::copyToByteArray(b, value, sizeof(double), dataOffset);

              /* Update SMA */
              auto &smaMin = reinterpret_cast<double &>(b[smaPos]);
              auto &smaMax = reinterpret_cast<double &>(b[smaPos + sizeof(double)]);
              if (smaMin > value || cnt == 0) smaMin = value;
              if (smaMax < value || cnt == 0) smaMax = value;

              /* Set new positions and sizes */
              recordOffset += sizeof(double);
              recordSize += sizeof(double);
              pTupleOffsets[idx] = dataOffset;

            }
              break;

            case ColumnInfo::String_Type: {
              /* Get Record Value */
              auto iterBegin = buf.cbegin() + recordOffset;
              const auto value = deserialize<std::string>(iterBegin, buf.end());
              const auto cValue = value.c_str();
              const auto stringSize = value.size() + 1;

              /* Insert Data - Get target position */
              const auto targetOffsetPos = dataPos + cnt * gOffsetSize;
              uint16_t targetDataPos;
              if (cnt == 0) {
                const auto end_minipage =
                  (tp.size() <= idx + 1) ? gBlockSize : reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos
                    + (idx + 1) * gAttrOffsetSize]);
                targetDataPos = end_minipage - stringSize;
              } else /* cnt != 0 */{
                const auto &last_offset = reinterpret_cast<const uint16_t &>(b[targetOffsetPos - gOffsetSize]);
                targetDataPos = last_offset - stringSize;
              }

              /* Insert Data - Set offset and string data */
              detail::copyToByteArray(b, targetDataPos, gOffsetSize, targetOffsetPos);
              detail::copyToByteArray(b, *cValue, stringSize, targetDataPos);

              /* Update SMA */
              auto &smaMinPos = reinterpret_cast<uint16_t &>(b[smaPos]);
              auto &smaMaxPos = reinterpret_cast<uint16_t &>(b[smaPos + gOffsetSize]);
              if (cnt != 0) {
                std::string smaMin(reinterpret_cast<const char (&)[]>(b[smaMinPos]));
                std::string smaMax(reinterpret_cast<const char (&)[]>(b[smaMaxPos]));
                if (smaMin > value) smaMinPos = targetDataPos;
                else if (smaMax < value) smaMaxPos = targetDataPos;
              } else /* cnt == 0 */{
                smaMinPos = targetDataPos;
                smaMaxPos = targetDataPos;
              }

              /* Set new positions and sizes */
              recordOffset += stringSize - 1 + sizeof(uint64_t);
              recordSize += stringSize + gOffsetSize;
              pTupleOffsets[idx] = targetDataPos;

            }
              break;

            default:throw TableException("unsupported column type\n");
          } /* end of switch */
          ++idx;
        } /* end of for */
        ++cnt;
        freeSpace -= recordSize;
        /* Insert into index structure */
        root->index->insert(key, PTuple<Tuple, KeyType>(targetNode, pTupleOffsets));
        /* Add key to KeyVector */
        targetNode->keys.get_rw()[cnt - 1] = key;
        /* Update histogram */
        const auto xtr = static_cast<uint32_t>(getBDCCFromTuple(tp).to_ulong());
        targetNode->histogram.get_rw()[xtr]++;
      }); /* end of transaction */

    } catch (std::exception &te) {
      std::cerr << te.what() << '\n' << "Inserting Tuple failed: " << tp << '\n';
      detail::mybacktrace();
    }

    return 1;
  }

  std::pair<DataNodePtr, DataNodePtr> splitBlock(DataNodePtr &oldNode) {
    auto pop = pool_by_vptr(this);
    const auto &tInfo = *this->root->tInfo;
    const auto &block0 = oldNode->block.get_ro();

    /* Calculate new ranges from histogram (at half for the beginning) */
    const auto &ddcMin = reinterpret_cast<const uint32_t &>(block0[gDDCRangePos1]);
    const auto &ddcMax = reinterpret_cast<const uint32_t &>(block0[gDDCRangePos2]);
    const auto splitValue = oldNode->calcAverageBDCC();
//    const auto splitValue = ddcMin + (ddcMax - ddcMin) / 2;
    PLOG("Splitting at: " << splitValue << " (" << ddcMin << ", " << ddcMax << ")");

    DataNodePtr newNode1;
    DataNodePtr newNode2;

    /* Create two new blocks */
    newNode1 = make_persistent<struct DataNode<KeyType>>();
    newNode2 = make_persistent<struct DataNode<KeyType>>();
    newNode1->block.get_rw() = initBlock(ddcMin, splitValue);
    if(splitValue==ddcMax) {
      newNode2->block.get_rw() = initBlock(ddcMax, ddcMax);
    } else {
      newNode2->block.get_rw() = initBlock(splitValue + 1, ddcMax);
    }
    const auto &block1 = newNode1->block.get_ro();
    const auto &block2 = newNode2->block.get_ro();

    /* Get, Calculate BDCC, Insert and delete all current values to corresponding block */
    // TODO: what about deleted records? Extra Bitmap?
    const auto &cnt = reinterpret_cast<const uint16_t &>(block0[gCountPos]);
    for (auto tuplePos = 0u; tuplePos < cnt; tuplePos++) {
      std::array<uint16_t, Tuple::NUM_ATTRIBUTES> pTupleOffsets;
      const auto &key = oldNode->keys.get_ro()[tuplePos];

      auto attributeIdx = 0u;
      for (auto &c : tInfo) {
        const auto &dataPos =
          reinterpret_cast<const uint16_t &>(block0[gDataOffsetPos + attributeIdx * gAttrOffsetSize]);
        switch (c.getType()) {
          case ColumnInfo::Int_Type: {
            pTupleOffsets[attributeIdx] = dataPos + tuplePos * sizeof(int32_t);
          }
            break;
          case ColumnInfo::Double_Type: {
            pTupleOffsets[attributeIdx] = dataPos + tuplePos * sizeof(double);
          }
            break;
          case ColumnInfo::String_Type: {
            pTupleOffsets[attributeIdx] =
              reinterpret_cast<const uint16_t &>(block0[dataPos + tuplePos * gOffsetSize]);
          }
            break;
          default:throw TableException("unsupported column type\n");
        } /* end of column type switch */
        ++attributeIdx;
      } /* end of attribute loop */

      const auto oldPTuple = PTuple<Tuple, KeyType>(oldNode, pTupleOffsets);

      /* Insert into correct new Block depending on BDCC value */
      root->index->erase(key);
      const auto tp = oldPTuple.createTuple();
      if(splitValue==ddcMax) {
        (tuplePos<cnt/2)? insertTuple(key, *tp, newNode1) : insertTuple(key, *tp, newNode2);
      } else {
        const auto xtr = static_cast<uint32_t>(getBDCCFromTuple(*tp).to_ulong());
        (xtr <= splitValue) ? insertTuple(key, *tp, newNode1) : insertTuple(key, *tp, newNode2);
      }
    } /* end of tuple loop */

    // adapt pointers
    if (root->dataNodes == oldNode) {
      root->dataNodes = newNode1;
    } else {
      auto prevBlock = root->dataNodes;
      while (prevBlock->next != oldNode) {
        prevBlock = prevBlock->next;
      }
      prevBlock->next = newNode1;
    }
    newNode1->next = newNode2;
    newNode2->next = oldNode->next;
    oldNode->clear();

    return std::make_pair(newNode1, newNode2);
  }

  const std::bitset<32> getBDCCFromTuple(const Tuple& tp) const {
    //TODO: Just use integers and shift operations instead of bitset
    const auto &bdccInfo = *this->root->bdccInfo;
    const auto &tInfo = *this->root->tInfo;
    auto dimCnt = 0u;        //< dimension column counter
    std::bitset<32> xtr = 0; //< extracted bdcc value

    for (const auto &dim: bdccInfo) {
      const auto &value = ns_types::dynamic_get(std::get<0>(dim), tp);
      const auto &nBits = std::get<1>(dim);
      const auto &mask = ((1L << nBits) - 1) << (21 - nBits);     // based on maximum inserted value (1000000)
//      const auto mask = (1L << nBits) - 1;                        // least significant bits
//      const auto mask = ~((1L << (sizeof(int) * 8 - nBits)) - 1); // most significant bits
      const auto &mapping = std::get<2>(dim);

      /* Calculate tuples bin for current dimension */
      int x = 0;
      if (value.type() == typeid(std::string)) {
        x = *reinterpret_cast<const int *>(boost::relaxed_get<std::string>(value).c_str()) & mask;
      } else if (value.type() == typeid(int)) {
        x = boost::relaxed_get<int>(value) & mask;
      } else if (value.type() == typeid(double)) {
        x = (int) boost::relaxed_get<double>(value) & mask;
      }
      x = x >> (21 - nBits); // realign the bits;

      /* Map the tuples bin to the dimensions BDCC positions */
      auto j = nBits - 1;
      auto dimXtr = mapping;
      for (auto i = 31; i >= 0; --i) {
        dimXtr[i] = ((x >> j) & 1) & mapping[i];
        j -= mapping[i];
      }
      xtr |= dimXtr;
    }
//    PLOG("Tuple:" << tp << ", BDCC Value: " << xtr);
    return xtr;
  }

  const std::vector<persistent_ptr<DataNode<KeyType>>> getCandidateBlocks(const ColumnRangeMap &predicates) const {
    std::vector<persistent_ptr<DataNode<KeyType>>> candidates;
    auto currentNode = root->dataNodes;
    const auto &tInfo = *root->tInfo;
    auto bCnt = 0u;
    while (currentNode!= nullptr) {
      ++bCnt;
      const auto &b = currentNode->block;
      auto inRange = true;

      for(const auto &p: predicates) {
        const auto &smaPos = reinterpret_cast<const uint16_t &>(b.get_ro()[gSmaOffsetPos + p.first * gAttrOffsetSize]);
        switch (tInfo.columnInfo(p.first).getType()) {
          case ColumnInfo::Int_Type: {
            const auto &smaMin = reinterpret_cast<const int32_t &>(b.get_ro()[smaPos]);
            const auto &smaMax = reinterpret_cast<const int32_t &>(b.get_ro()[smaPos + sizeof(int32_t)]);
            PLOG("predicate range: " << boost::get<int>(p.second.first) << '-'
                                    << boost::get<int>(p.second.second)
                                    << ", block range: " << smaMin << '-' << smaMax)
            if (boost::get<int>(p.second.second) < smaMin || boost::get<int>(p.second.first) > smaMax) {
              inRange = false;
              goto marker;
            }
          }
            break;
          case ColumnInfo::Double_Type: {
            const auto &smaMin = reinterpret_cast<const double &>(b.get_ro()[smaPos]);
            const auto &smaMax = reinterpret_cast<const double &>(b.get_ro()[smaPos + sizeof(double)]);
            PLOG("predicate range: " << boost::get<double>(p.second.first) << '-'
                                    << boost::get<double>(p.second.second)
                                    << ", block range: " << smaMin << '-' << smaMax)
            if (boost::get<double>(p.second.second) < smaMin || boost::get<double>(p.second.first) > smaMax) {
              inRange = false;
              goto marker;
            }
          }
            break;
          case ColumnInfo::String_Type: {
            const auto &smaMinPos = (uint16_t &) b.get_ro()[smaPos];
            const auto &smaMaxPos = (uint16_t &) b.get_ro()[smaPos + gOffsetSize];
            const auto smaMin(reinterpret_cast<const char (&)[]>(b.get_ro()[smaMinPos]));
            const auto smaMax(reinterpret_cast<const char (&)[]>(b.get_ro()[smaMaxPos]));
            PLOG("predicate range: " << boost::get<std::string>(p.second.first) << '-'
                                    << boost::get<std::string>(p.second.second)
                                    << ", block range: " << smaMin << '-' << smaMax)
            if (boost::get<std::string>(p.second.second) < smaMin || boost::get<std::string>(p.second.first) > smaMax) {
              inRange = false;
              goto marker;
            }
          }
            break;
          default:throw TableException("unsupported column type\n");
        }
      }
      marker:;
      if (inRange) candidates.push_back(currentNode);
      currentNode = currentNode->next;
    }
    PLOG("global blocks: " << bCnt);
    PLOG("candidate blocks: " << candidates.size());
    return candidates;
  }

  const bool isPTupleinRange(const PTuple<Tuple, KeyType> &ptp, const ColumnRangeMap &predicates) const {
    const auto &tInfo = *root->tInfo;
    const auto &b = ptp.getNode()->block.get_ro();
    auto inRange = true;
    for(const auto &p: predicates) {
      auto offset = ptp.getOffsetAt(p.first);

      switch (tInfo.columnInfo(p.first).getType()) {
        case ColumnInfo::Int_Type: {
          const auto &value = reinterpret_cast<const int32_t &>(b[offset]);
          if (boost::get<int>(p.second.second) < value || boost::get<int>(p.second.first) > value) {
            return false;
          }
        }
          break;
        case ColumnInfo::Double_Type: {
          const auto &value = reinterpret_cast<const double &>(b[offset]);
          if (boost::get<double>(p.second.second) < value || boost::get<double>(p.second.first) > value) {
            return false;
          }
        }
          break;
        case ColumnInfo::String_Type: {
          const std::string value(reinterpret_cast<const char (&)[]>(b[offset]));
          if (boost::get<std::string>(p.second.second) < value || boost::get<std::string>(p.second.first) > value) {
            return false;
          }
        }
          break;
        default:throw TableException("unsupported column type\n");
      }
    }
    return true;
  }

  const bool findInsertNodeOrSplit(DataNodePtr &node, const Tuple &tp) const {

    auto ddc_min = reinterpret_cast<const uint32_t &>(node->block.get_ro()[gDDCRangePos1]);
    auto ddc_max = reinterpret_cast<const uint32_t &>(node->block.get_ro()[gDDCRangePos2]);
    auto enoughSpace = hasEnoughSpace(node, tp);
    auto splitNode = node;

    /* Loop for special case where multiple nodes share the same bdcc value */
    while (ddc_min == ddc_max && !enoughSpace) {
      splitNode = splitNode->next;
      ddc_min = reinterpret_cast<const uint32_t &>(splitNode->block.get_ro()[gDDCRangePos1]);
      ddc_max = reinterpret_cast<const uint32_t &>(splitNode->block.get_ro()[gDDCRangePos2]);
      enoughSpace = hasEnoughSpace(splitNode, tp);
    }
    /* reset if all nodes with same bdcc value are full */
    auto xtr = static_cast<uint32_t>(getBDCCFromTuple(tp).to_ulong());
    if (xtr < ddc_min) {
      splitNode = node;
      enoughSpace = false;
    }

    /* set results */
    node = splitNode;
    return enoughSpace ? false : true;
  }

  const bool hasEnoughSpace(const DataNodePtr &node, const Tuple &tp) const {
    StreamType buf;
    tp.serializeToStream(buf);

    const auto b = node->block.get_ro();

    /* first check total space of node */
    const auto globalSpace =  reinterpret_cast<const uint16_t &>(b[gFreeSpacePos]);
    if (globalSpace < buf.size())
      return false;

    /* then check space of minipages of the node */
    auto recordOffset = 1u;
    auto idx = 0u;
    const auto &cnt = reinterpret_cast<const uint32_t &>(b[gCountPos]);
    for (auto &c : *root->tInfo) {
      const auto &dataPos = reinterpret_cast<const uint16_t &>(b[gDataOffsetPos + idx * gAttrOffsetSize]);
      switch (c.getType()) {

        case ColumnInfo::Int_Type: {
          const auto nextMiniPageStart =
            (tp.size() == idx + 1) ? gBlockSize : reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos
              + (idx + 1) * gAttrOffsetSize]);
          const auto freeSpaceMiniPage = nextMiniPageStart - dataPos - (cnt * sizeof(int32_t));
          if (freeSpaceMiniPage < sizeof(int32_t)) return false;
          recordOffset += sizeof(int32_t);
        } break;

        case ColumnInfo::Double_Type: {
          const auto nextMiniPageStart =
            (tp.size() == idx + 1) ? gBlockSize : reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos
              + (idx + 1) * gAttrOffsetSize]);
          const auto freeSpaceMiniPage = nextMiniPageStart - dataPos - (cnt * sizeof(double));
          if (freeSpaceMiniPage < sizeof(double)) return false;
          recordOffset += sizeof(double);
        } break;

        case ColumnInfo::String_Type: {
          uint16_t freeSpaceMiniPage;
          auto iterBegin = buf.cbegin() + recordOffset;
          const auto value = deserialize<std::string>(iterBegin, buf.end());
          const auto cValue = value.c_str();
          const auto stringSize = value.size() + 1;

          if (cnt != 0) {
            const auto currentOffsetPos = dataPos + cnt * gOffsetSize;
            const auto &currentOffset = reinterpret_cast<const uint16_t &>(b[currentOffsetPos - gOffsetSize]);
            freeSpaceMiniPage = currentOffset - currentOffsetPos;
          } else {
            const auto nextMiniPageStart =
              (tp.size() == idx + 1) ? gBlockSize : reinterpret_cast<const uint16_t &>(b[gSmaOffsetPos
                + (idx + 1) * gAttrOffsetSize]);
            freeSpaceMiniPage = nextMiniPageStart - dataPos;
          }
          if (freeSpaceMiniPage < stringSize + gOffsetSize) return false;
          recordOffset += stringSize - 1 + sizeof(uint64_t);
        } break;
        default: break;
      }
      ++idx;
    }
    return true;
  }


  }; /* class persistent_table */

}} /* namespace pfabric::nvm */

#endif /* PTable_hpp_ */