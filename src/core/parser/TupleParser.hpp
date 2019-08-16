/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef TUPLEPARSER_HPP_
#define TUPLEPARSER_HPP_

#include "SelectAttributeParser.hpp"

#include <boost/core/ignore_unused.hpp>
#include <cassert>


namespace pfabric {


/**
 * @brief TupleParser is a helper for parsing tuple attributes from strings.
 *
 * This class can be used to parse a tuple from any kind of supported representation
 * into its type with typed attributes. A tuple is basically a sequence of attributes
 * of different types which can be parsed from a common representation like a string.
 * Therefore, this class just parses each single attribute using an attribute parser
 * that is available for the requested representation. For a list of currently supported
 * input types, see the @c SelectAttributeParser meta function.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
struct TupleParser {

	/**
	 * @brief Parses a given string tuple into a given tuple type.
	 *
	 * @tparam TupleType
	 *    the tuple type to be parsed
	 * @param[out] tup
	 *    reference to the target tuple for storing the parsed value
	 * @param[in] data
	 *    a string tuple comprising the data to be parsed
	 */
	template< TupleSize NumAttributes, typename TupleType >
	static void parseTuple( TupleType& tup, const StringTuple& data) {
//		typedef ns_types::TupleSize< TupleType > MaxIndex;
		// TODO make this a more verbose exception
		assert( data.size() == NumAttributes );
		if( NumAttributes > 0 ) {
			parseTupleImpl( std::integral_constant<AttributeIdx, NumAttributes-1>(), tup, data );
		}
	}

	/**
	 * @brief Parses a given tuple of string references into a given tuple type.
	 *
	 * TODO use a less fragile type than raw pointers for representing an array!
	 *      The number of attributes represented by the type is lost which may
	 *      easily lead to undefined behavior when using the []-operator for accessing
	 *      the attribute value!
	 *      When using a general collection/container we can use a unified interface
	 *      and get rid of these overloads using a template parseTuple method with
	 *      the representation to be parsed as parameter like its done for attributes
	 *      in parseAttribute().
	 *
	 * @tparam TupleType
	 *    the tuple type to be parsed
	 * @param[out] tup
	 *    reference to the target tuple for storing the parsed value
	 * @param[in] data
	 *    a string tuple comprising the data to be parsed
	 */
	template< TupleSize NumAttributes, typename TupleType >
	static void parseTuple( TupleType& tup, const StringRef* data) {
		assert( data != nullptr );
		parseTupleImpl( std::integral_constant<AttributeIdx, NumAttributes-1>(), tup, data );
	}

private:


	/**
	 * @brief Parses the given attribute representation into a value of type @c AttributeType.
	 *
	 * This method selects an appropriate attribute parser for the requested type
	 * and forwards the @c parse() call there.
	 *
	 * @tparam AttributeType
	 *    the target type of the attribute to be parsed
	 * @tparam AttributeRepresentation
	 *    the type of the attribute representation which shall be parsed
	 * @param[in] input
	 *    the representation of the attribute value to be parsed
	 * @param[out] out
	 *    a reference to a value of the requested type into which the parsed attribute is stored
	 */
	template<
		typename AttributeType,
		typename AttributeRepresentation
	>
	static inline void parseAttribute( AttributeRepresentation&& input, AttributeType& out ) {
		typedef typename SelectAttributeParser<
			AttributeType, AttributeRepresentation
		>::type AttributeParser;

		AttributeParser::parse( std::forward< AttributeRepresentation >( input ), out );
	}

	/**
	 * @brief Helper functions for parsing a vector of strings into a tuple of the given type.
	 *
	 * General overload for more than one value to be parsed. The values will be parsed with
	 * recursively with decreasing calling the parse function until the last attribute index is reached.
	 *
	 * @tparam TupleType
	 *    the final tuple type to be parsed (with typed attributes)
	 * @tparam StringTupleType
	 *    the type of the string tuple representation which shall be parsed
	 * @tparam CurrentIndex
	 *    the index of the current attribute to be parsed
	 * @param[in] idx
	 *    compile-time constant representing the current index of the attribute to be parsed
	 *    (ignored here, just used for dispatching)
	 * @param[out] tup
	 *    reference to the target tuple for storing the parsed value
	 * @param[in] data
	 *    a string tuple comprising the data to be parsed
	 */
	template<
		typename TupleType,
		typename StringTupleType,
		AttributeIdx CurrentIndex
	>
	static void parseTupleImpl( std::integral_constant<AttributeIdx, CurrentIndex> idx, TupleType& tup,
		const StringTupleType& data) {
		boost::ignore_unused( idx );
		parseAttribute( data[CurrentIndex], ns_types::get<CurrentIndex>(tup) );
		parseTupleImpl( std::integral_constant<AttributeIdx, CurrentIndex - 1>(), tup, data );
	}

	/**
	 * @brief Helper functions for parsing a vector of strings into a tuple of the given type.
	 *
	 * Overload for the first attribute in the tuple, terminates recursion.
	 *
	 * @tparam TupleType
	 *    the tuple type to be parsed
	 * @tparam StringTupleType
	 *    the type of the string tuple representation which shall be parsed
	 * @param[in] idx
	 *    compile-time constant representing the current index of the attribute to be parsed
	 *    (ignored here, just used for dispatching)
	 * @param[out] tup
	 *    reference to the target tuple for storing the parsed value
	 * @param[in] data
	 *    a string tuple comprising the data to be parsed
	 */
	template<
		typename TupleType,
		typename StringTupleType
	>
	static void parseTupleImpl( std::integral_constant<AttributeIdx, 0> idx, TupleType& tup,
		const StringTupleType& data) {
		boost::ignore_unused( idx );
		parseAttribute( data[0], ns_types::get<0>(tup) );
	}
};

} /* end namespace pquery */


#endif /* TUPLEPARSER_HPP_ */
