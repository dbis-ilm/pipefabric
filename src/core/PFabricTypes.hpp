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

#ifndef PFabricTypes_hpp_
#define PFabricTypes_hpp_

#include "libcpp/types/types.hpp"

#include <string>
#include <vector>

namespace pfabric {

  typedef unsigned long DefaultKeyType;


/// a name for a query operator parameter
typedef std::string ParameterName;

/// a value for a query operator parameter
typedef std::string ParameterValue;

/// a type for representing the number of attributes in a tuple
typedef std::size_t TupleSize;

/// a type used to represent an index of a specific tuple attribute
typedef std::size_t AttributeIdx;


/**
 * Typedef for the timestamps associated with each tuple. A timestamp represents the number
 * of microseconds since 01/01/1970 indicating when the tuple arrived in the system.
 */
typedef unsigned long long Timestamp;

/// a number for limiting the number of produced tuples
typedef std::size_t TupleLimit;

/// the length of a sliding window
typedef unsigned int SlideLength;

/// unique identifier of a transaction
typedef unsigned long TransactionID;

/// vector of strings
typedef std::vector< std::string > StringTuple;


class Punctuation;

/// a shared pointer to a stream punctuation
typedef std::shared_ptr< Punctuation > PunctuationPtr;


#if 0
/// a pointer to a generic tuple
template< typename TupleType >
using TuplePtr = boost::intrusive_ptr< TupleType >;
#endif

template< typename T >
using SmartPtr = boost::intrusive_ptr< T >;

class BaseAggregateState;

/// a shared pointer to an aggregation state
typedef std::shared_ptr<BaseAggregateState> AggregateStatePtr;


template<class T>
class GroupedAggregateState;

/// a shared pointer to a grouped aggregation state
template<class T>
using GroupedAggregateStatePtr = std::shared_ptr< GroupedAggregateState< T > >;


  // possible types of triggers for producing results in aggregation.
  enum AggregationTriggerType {
    TriggerAll,
    TriggerByCount,
    TriggerByTime,
    TriggerByTimestamp
  };


typedef uint8_t Byte;

enum class TupleType : Byte {
    Normal,
    Punctuation,
};

} /* end namespace pfabric */


#endif /* PFabricTypes_hpp_ */
