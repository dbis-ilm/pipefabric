/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LogBuffer_hpp_
#define LogBuffer_hpp_

#include <list>
#include <unordered_map>

#include "core/PFabricTypes.hpp"

namespace pfabric {

  enum class LogOp { Insert, Update, Delete };

  template<typename KeyType, typename RecordType>
  struct LogEntry {
    using RecordTypePtr = std::shared_ptr<RecordType>;

      LogOp logOp;
      KeyType key;
      RecordTypePtr recordPtr;
      LogEntry(LogOp op, const KeyType& k, const RecordType& rec) :
        logOp(op), key(k), recordPtr(std::make_shared<RecordType>(rec)) {}

        LogEntry(LogOp op, const KeyType& k) :
          logOp(op), key(k) {}
  };

  template<typename KeyType, typename RecordType>
  class LogBuffer {
  public:
    using LogEntries = std::list<LogEntry<KeyType, RecordType>>;
    using LogMap = std::unordered_map<TransactionID, LogEntries>;
    using Iterator = typename LogEntries::const_iterator;

  LogBuffer() {}

  void append(const TransactionID& txID, LogOp op, const KeyType& k) {
    LogEntry<KeyType, RecordType> entry(op, k);
    auto iter = buffer.find(txID);
    if (iter != buffer.end())
      iter->second.push_back(entry);
    else
      buffer.insert({ txID, LogEntries(1, entry) });
  }

  void append(const TransactionID& txID, LogOp op, const KeyType& k, const RecordType& r) {
    LogEntry<KeyType, RecordType> entry(op, k, r);
    auto iter = buffer.find(txID);
    if (iter != buffer.end())
      iter->second.push_back(entry);
    else
      buffer.insert({ txID, LogEntries(1, entry) });
  }

  Iterator begin(const TransactionID& txID) const {
    auto iter = buffer.find(txID);
    assert (iter != buffer.end());
   return iter->second.begin();
  }

  Iterator end(const TransactionID& txID) const {
    auto iter = buffer.find(txID);
    assert (iter != buffer.end());
    return iter->second.end();
  }

  void cleanup(const TransactionID& txID) {
    buffer.erase(txID);
  }

private:
    LogMap buffer;
  };

}

#endif
