/*
 * PipeFabricTypes.hpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#ifndef PIPEFABRICTYPES_HPP_
#define PIPEFABRICTYPES_HPP_

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


/// vector of strings
typedef std::vector< std::string > StringTuple;


class Punctuation;

/// a shared pointer to a stream punctuation
typedef std::shared_ptr< Punctuation > PunctuationPtr;


/// a pointer to a generic tuple
template< typename TupleType >
using TuplePtr = boost::intrusive_ptr< TupleType >;


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


#endif /* PIPEFABRICTYPES_HPP_ */
