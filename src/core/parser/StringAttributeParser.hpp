/*
 * StringAttributeParser.hpp
 *
 *  Created on: Apr 2, 2015
 *      Author: fbeier
 */

#ifndef STRINGATTRIBUTEPARSER_HPP_
#define STRINGATTRIBUTEPARSER_HPP_

#include "AttributeParserBase.hpp"
#include "core/TimestampHelper.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>
#include <cstdlib>
#include <cassert>

#ifdef USE_BOOST_SPIRIT_PARSER
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
#endif


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
class StringAttributeParser :
	public AttributeParserBase< AttributeType, StringAttributeParser< AttributeType > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef AttributeType Attribute;


	/**
	 * @brief Parses the given string inp into a value of type @c AttributeType.
	 *
	 * General implementation. Specializations need to be provided for handling
	 * types that cannot be handled by @c boost::lexical_cast, or where a more
	 * efficient solution can be provided.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value of the requested type into which the parsed string is stored
	 */
	static inline void parse( const std::string& input, Attribute& out ) {
		try {
			out = boost::lexical_cast< Attribute >(input);
		}
		catch(boost::bad_lexical_cast& exc) {
			BOOST_LOG_TRIVIAL(error) << "TupleHelper::fill: bad lexical cast for: '" << input << "'";
			throw exc;
		}
	}
};


/////// specialized template instantiations for basic attribute parsers

/**
 * @brief Attribute parser for an integer.
 */
template<>
class StringAttributeParser< int > :
	public AttributeParserBase< int, StringAttributeParser< int > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef int Attribute;


	/**
	 * @brief Parse a string to an integer.
	 *
	 * TODO The atoi function is unsafe and might result in undefined behavior when the
	 *      converted value would be out of the range of possibly representable values.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const std::string& input, Attribute& out ) {
#ifdef USE_BOOST_SPIRIT_PARSER
		qi::parse(input.begin(), input.end(), qi::int_, out);
#else
		out = atoi(input.c_str());
#endif
	}
};


/**
 * @brief Attribute parser for a long value.
 */
template<>
class StringAttributeParser< long > :
	public AttributeParserBase< long, StringAttributeParser< long > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef long Attribute;


	/**
	 * @brief Parse a string to a long value.
	 *
	 * TODO The atol function is unsafe and might result in undefined behavior when the
	 *      converted value would be out of the range of possibly representable values.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const std::string& input, Attribute& out ) {
#ifdef USE_BOOST_SPIRIT_PARSER
		qi::parse(input.begin(), input.end(), qi::long_, out);
#else
		out = atol(input.c_str());
#endif
	}
};

/**
 * @brief Attribute parser for a double.
 */
template<>
class StringAttributeParser< double >  :
	public AttributeParserBase< double, StringAttributeParser< double > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef double Attribute;


	/**
	 * @brief Parse a string to a double.
	 *
	 * TODO The atof function is unsafe and might result in undefined behavior when the
	 *      converted value would be out of the range of possibly representable values.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const std::string& input, Attribute& out ) {
#ifdef USE_BOOST_SPIRIT_PARSER
		qi::parse(input.begin(), input.end(), qi::double_, out);
#else
		out = atof(input.c_str());
#endif
	}
};


/**
 * @brief Attribute parser for a string.
 */
template<>
class StringAttributeParser< std::string >  :
	public AttributeParserBase< std::string, StringAttributeParser< std::string > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef std::string Attribute;


	/**
	 * @brief Parse a string to a string.
	 *
	 * This method just copies the string into the target attribute.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const std::string& input, Attribute& out ) {
		out = input;
	}
};


/**
 * @brief Attribute parser for a time value.
 */
template<>
class StringAttributeParser< boost::posix_time::ptime >  :
	public AttributeParserBase< std::string, StringAttributeParser< boost::posix_time::ptime > >
{
public:

	/// the attribute type to which can be parsed with this class
	typedef boost::posix_time::ptime Attribute;


	/**
	 * @brief Parse a string to a time value.
	 *
	 * @param[in] input
	 *    a string representation of the value
	 * @param[out] out
	 *    a reference to a value into which the parsed string is stored
	 */
	static inline void parse( const std::string& input, Attribute& out ) {
		out = TimestampHelper::timestampToPtime(
			TimestampHelper::parseTimestamp(input)
		);
	}
};

} /* end namespace pquery */


#endif /* STRINGATTRIBUTEPARSER_HPP_ */
