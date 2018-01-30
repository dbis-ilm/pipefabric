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

#ifndef Table_hpp_
#define Table_hpp_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <exception>
#include <iterator>

#include <mutex>
#include <shared_mutex>

#include <boost/signals2.hpp>

#include "fmt/format.h"

#include "table/TableException.hpp"
#include "table/BaseTable.hpp"

#if defined(USE_ROCKSDB_TABLE)

#include "RDBTable.hpp"

namespace pfabric {
template <typename RecordType, typename KeyType = DefaultKeyType>
using Table = pfabric::RDBTable<RecordType, KeyType>;
}

#elif defined(USE_NVML_TABLE)

#include "NVMTable.hpp"

namespace pfabric {
template <typename RecordType, typename KeyType = DefaultKeyType>
using Table = pfabric::NVMTable<RecordType, KeyType>;
}

#else

#include "HashMapTable.hpp"

namespace pfabric {
template <typename RecordType, typename KeyType = DefaultKeyType>
using Table = pfabric::HashMapTable<RecordType, KeyType>;
}

#endif

#include "table/TxTable.hpp"

#endif
