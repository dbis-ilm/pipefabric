/*
 * SelectAttributeParser.hpp
 *
 *  Created on: Apr 2, 2015
 *      Author: fbeier
 */

#ifndef SELECTATTRIBUTEPARSER_HPP_
#define SELECTATTRIBUTEPARSER_HPP_

#include "StringAttributeParser.hpp"
#include "StringRefAttributeParser.hpp"

#include <boost/mpl/map.hpp>
#include <boost/mpl/quote.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/assert.hpp>


namespace pfabric {

namespace impl {

/**
 * @brief A map with all parsable types as key and corresponding parser templates as values.
 *
 * Register new parsable types here. The corresponding parser must be an unary metafunction class
 * that returns an attribute parser for a specific attribute type.
 */
typedef boost::mpl::map<
	boost::mpl::pair< std::string, boost::mpl::quote1< StringAttributeParser > >,
	boost::mpl::pair< StringRef, boost::mpl::quote1< StringRefAttributeParser > >
> AvailableAttributeParsers;

} /* end namespace impl */


/**
 * @brief Meta function that gets a parser class parsing a single attribute.
 *
 * This meta function returns a parser class that can be used to parse an attribute
 * of type @c AttributeType from a @c AttributeRepresentation
 *
 * @tparam AttributeType
 *    the target type of the attribute to be parsed
 * @tparam AttributeRepresentation
 *    the representation of the attribute to be parsed
 */
template<
	typename AttributeType,
	typename AttributeRepresentation
>
class SelectAttributeParser {
private:

	// make sure that a parser for the representation was registered above
	BOOST_MPL_ASSERT_MSG(
		(boost::mpl::has_key< impl::AvailableAttributeParsers, AttributeRepresentation >::value),
		NO_ATTRIBUTE_PARSER_REGISTERED_FOR_REQUESTED_REPRESENTATION,
		(AttributeRepresentation)
	);

	/// get the parser template for the requested attribute representation
	typedef typename boost::mpl::at<
		impl::AvailableAttributeParsers, AttributeRepresentation
	>::type UntypedAttributeParser;

	/// specialize it for the requested attribute type
	typedef typename boost::mpl::apply<
		UntypedAttributeParser, AttributeType
	>::type AttributeParser;

public:

	/// return the attribute requested parser
	typedef AttributeParser type;
};


///////  template specializations for handling const and reference representation types  ///////

template<
	typename AttributeType,
	typename AttributeRepresentation
>
class SelectAttributeParser< AttributeType, const AttributeRepresentation > :
	public SelectAttributeParser< AttributeType, AttributeRepresentation >
{};

template<
	typename AttributeType,
	typename AttributeRepresentation
>
class SelectAttributeParser< AttributeType, AttributeRepresentation& > :
	public SelectAttributeParser< AttributeType, AttributeRepresentation >
{};

template<
	typename AttributeType,
	typename AttributeRepresentation
>
class SelectAttributeParser< AttributeType, AttributeRepresentation&& > :
	public SelectAttributeParser< AttributeType, AttributeRepresentation >
{};

} /* end namespace pquery */


#endif /* SELECTATTRIBUTEPARSER_HPP_ */
