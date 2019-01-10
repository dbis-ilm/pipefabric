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

#ifdef SUPPORT_MATRICES
#include "core/StreamElementTraits.hpp"
#include "matrix/Matrix.hpp"
#include "matrix/VectorParser.hpp"
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

#ifdef SUPPORT_MATRICES
/**
 * @brief Atribute parser for sparse vector
 **/

template<typename CellType>
class StringAttributeParser<pfabric::SparseVector<CellType> > :
	public AttributeParserBase<pfabric::SparseVector<CellType>, StringAttributeParser<pfabric::SparseVector<CellType> > > {
public:
  // the attribute should be initialized
  typedef pfabric::SparseVector<CellType> Attribute;

  /**
   * @brief Parse reads vector of values from an attribute of tuple (e.g. TuplePtr< int, int, v1 v2 v3 ... >)
   *
   * @param[in] input
   *	string of values (v1 v2 v3 ...)
   * @param[out] matrix
   *	the matrix which should be initialized by string of values from a tuple.
   */
  static inline
  void parse(const std::string &input, Attribute &vector) {
	VectorParser::parse(input, vector);
  }
};

/**
 * @brief Atribute parser for dense vector
 */

template<typename CellType, int Rows, int Cols>
class StringAttributeParser<pfabric::DenseMatrix<CellType, Rows, Cols> > :
	public AttributeParserBase<pfabric::DenseMatrix<CellType, Rows, Cols>, StringAttributeParser<pfabric::DenseMatrix<CellType, Rows, Cols>>> {
public:
  // the attribute should be initialized
  typedef pfabric::DenseMatrix<CellType, Rows, Cols> Attribute;

  /**
	* @brief Parse reads vector of values from an attribute of tuple (e.g. TuplePtr< int, int, v1 v2 v3 ... >)
	*
	* @param[in] input
	*	string of values (v1 v2 v3 ...)
	* @param[out] matrix
	*	the matrix which should be initialized by string of values from a tuple.
	*/
	static inline
	void parse(const std::string &input, Attribute &vector) {
	  VectorParser::parse(input, vector);
	}
};
#endif

} /* end namespace pquery */


#endif /* STRINGATTRIBUTEPARSER_HPP_ */
