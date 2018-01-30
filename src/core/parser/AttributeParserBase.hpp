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

#ifndef ATTRIBUTEPARSERBASE_HPP_
#define ATTRIBUTEPARSERBASE_HPP_

#include <type_traits>


namespace pfabric {

/**
 * @brief A base class for a single attribute parser.
 *
 * Base class for all parser classes that can parse as single attribute value.
 * It uses CRTP to forward parsing calls to the actual parser implementation.
 * It adds a function that returns a parsed attribute value instead of storing
 * it in a variable reference.
 *
 * TODO The current implementation requires that the attributes of AttributeType
 *      are default-constructible since parsing is done inside an existing element
 *      via the assignment operator. (used to create a tuple from a sequence of strings
 *      with default constructing the tuple and filling its attributes afterwards)
 *      Maybe this could be improved with creating attributes directly from the string source
 *      (and a tuple accordingly).
 *
 * @tparam AttributeType
 *    the type of the attribute that is parsed
 */
template<
	typename AttributeType,
	typename AttributeParserImpl
>
class AttributeParserBase {
public:

	/// the parsed attribute type
	/// (cannot access typename AttributeParserImpl::Attribute since type is incomplete here)
	typedef AttributeType Attribute;


	/**
	 * @brief Parses the given string input into a value of type @c AttributeType.
	 *
	 * This method just forwards to the actual parser implementation using the CRTP pattern.
	 *
	 * @tparam AttributeType
	 *    the type of the attribute to be parsed
	 * @param[in] input
	 *    a string representation of the value
	 * @return the parsed value of the requested type
	 */
	template< typename AttributeRep >
	static inline Attribute parse( AttributeRep&& input ) {
		Attribute parsedAttribute;
		AttributeParserImpl::parse( std::forward< AttributeRep >( input ), parsedAttribute );
		return parsedAttribute;
	}
};

} /* end namespace pquery */


#endif /* ATTRIBUTEPARSERBASE_HPP_ */
