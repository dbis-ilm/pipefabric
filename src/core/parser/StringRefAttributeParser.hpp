/*
 * Copyright (c) 2014-16 The PipeFabric team,
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

#ifndef STRINGREFATTRIBUTEPARSER_HPP_
#define STRINGREFATTRIBUTEPARSER_HPP_

#include "StringRef.hpp"
#include "StringAttributeParser.hpp"


namespace pfabric {

/**
 * @brief A parser class for converting strings into typed attribute values.
 *
 * @tparam AttributeType
 *    the type of the attribute to be parsed
 */
template<
	typename AttributeType
>
class StringRefAttributeParser :
	public AttributeParserBase< AttributeType, StringRefAttributeParser< AttributeType > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef AttributeType Attribute;

	/**
	 * @brief Parse a given input into a value of type @c AttributeType
	 *
	 * General implementation which converts the @c input @c StringRef into a @c std::string
	 * object and just forwards the parsing to the @c StringAttributeParser.
	 *
	 * @tparam AttributeType
	 *    the type of the attribute to be parsed
	 * @param[in] input
	 *    a reference to the string representation of the value
	 * @param[out] out
	 *    a reference to a value of the requested type into which the parsed attribute is stored
	 */
	static inline void parse( const StringRef& input, Attribute& out ) {
		StringAttributeParser< Attribute >::parse(
			std::string( input.begin_, input.size_ ), out
		);
	}
};


/////// specialized template instantiations for basic attribute parsers

#ifdef USE_BOOST_SPIRIT_PARSER

/**
 * @brief Attribute parser for an integer.
 */
template<>
class StringRefAttributeParser< int > :
	public AttributeParserBase< int, StringRefAttributeParser< int > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef int Attribute;


	/**
	 * @brief Parse a string to an integer.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const StringRef& input, Attribute& out ) {
		qi::parse(input.begin(), input.end(), qi::int_, out);
	}
};


/**
 * @brief Attribute parser for a double.
 */
template<>
class StringRefAttributeParser< double >  :
	public AttributeParserBase< double, StringRefAttributeParser< double > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef double Attribute;


	/**
	 * @brief Parse a string to a double.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const StringRef& input, Attribute& out ) {
		qi::parse(input.begin(), input.end(), qi::double_, out);
	}
};

#endif


/**
 * @brief Attribute parser for a string.
 */
template<>
class StringRefAttributeParser< std::string >  :
	public AttributeParserBase< std::string, StringRefAttributeParser< std::string > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef std::string Attribute;


	/**
	 * @brief Parse a string to a string.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const StringRef& input, Attribute& out ) {
	  out = std::string( input.begin(), std::min(1023, input.size()) );
	}
};

} /* end namespace pquery */


#endif /* STRINGREFATTRIBUTEPARSER_HPP_ */
