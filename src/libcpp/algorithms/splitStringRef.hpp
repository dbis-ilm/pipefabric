/*
 * splitStringRef.hpp
 *
 *  Created on: Apr 28, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_ALGORITHMS_SPLITSTRINGREF_HPP_
#define LIBCPP_ALGORITHMS_SPLITSTRINGREF_HPP_

#include "libcpp/types/detail/SubstringRef.hpp"

#include <iterator>
#include <vector>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/split.hpp>


namespace ns_algorithms {

namespace impl {

/**
 * @brief Functor for converting an iterator range into a substring reference.
 *
 * This functor can be used to convert an iterator range comprising of a @c begin
 * and an @c end string-iterator into a corresponding @c SubstringRef which
 * is implemented in terms of a start offset and a length according to a string reference.
 *
 * @tparam String
 *    the string type used for the substring reference
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename String
>
class RangeToSubstringRef {
public:

	/// the substring reference type that is constructed
	typedef ns_types::SubstringRef< String > Substring;

	/**
	 * @brief Create a new transformation functor for converting a range into
	 *        substring references of the underlying @c str base string.
	 *
	 * @param[in] str
	 *    the underlying string as base which is referred by the constructed substring refs
	 */
	RangeToSubstringRef( String& str ) :
		mString( str ) {
	}

	/**
	 * @brief The conversion operator.
	 *
	 * @note The caller must make sure that the input @c range passed as argument is defined
	 *       on the same string reference that was used to construct this functor instance.
	 *       Otherwise, the iterators cannot be converted relative to the base strings @c begin().
	 *
	 * @tparam Range
	 *    the type of the iterator range to be converted
	 * @param[in] range
	 *    the string iterator range to be converted
	 * @return the converted substring reference as offset and length relative to the base string
	 */
	template< typename Range >
	Substring operator() ( const Range& range ) const {
		const auto& offset = std::distance( std::begin( mString ), range.begin() );
		const auto& length = range.size();
		return Substring( mString, offset, length );
	}

private:

	String& mString; /**< the string reference used as base for creating substring references */
};

} /* end namespace impl */


/**
 * @brief Split a given input string into a collection of substring references according to a predicate.
 *
 * # Abstract
 *
 * This method reads the @c input string and tokenizes it according to a custom @c splitPredicate.
 * For any character it returns true a new substring is formed from the last split point
 * (or the beginning of the string) to this character as a substring reference.
 * Since substring references are created, no data is copied, but the caller must
 * make sure that reference to the @c input string does not become invalid as long as
 * the substrings are processed further. The result substring references are appended to
 * a @c Container passed as @c results parameter.
 *
 * # Usage
 *
 * @code
 *    // a custom container for storing split results
 *    typedef ns_types::SubstringRef< const std::string > SubstringRef;
 *    std::list< SubstringRef > substringRefs;
 *
 *    std::string toSplit = "This is a test";
 *    // split the input strings when at spaces and return references to the substrings
 *    const auto& substrings = splitStringRef( toSplit,
 *        []( const char c ) -> bool {
 *            return c == 'i';
 *        }
 *    );
 * @endcode
 *
 * @tparam String
 *    the string type used as input
 * @tparam SplitPredicate
 *    the predicate type used for deciding about a split for each character in the input string
 * @tparam Container
 *    the container type where result substring references are appended to
 *    (must be supported by @c std::back_inserter)
 * @param[in] input
 *    reference to the string to be split
 * @param[in] splitPredicate
 *    the predicate that causes a split when returning true for a character in the input string
 * @param[out] results
 *    reference to the container that shall store the resulting string references
 * @return a vector with substring references into the input string representing the split results.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename String,
	typename SplitPredicate,
	typename Container
>
void splitStringRef( String& str, SplitPredicate splitPredicate, Container& results ) {
	typedef typename String::const_iterator StringIterator;
	typedef boost::iterator_range< StringIterator > StringIteratorRange;
	typedef std::vector< StringIteratorRange > SplitVector;
	typedef impl::RangeToSubstringRef< String > ToSubstringRef;

	// use boost split algorithm to split the input string into ranges according to the predicate
	SplitVector splitResults;
	boost::split( splitResults, str, splitPredicate, boost::token_compress_off );

	// transform the vector of substring ranges into substring references
	std::transform( splitResults.begin(), splitResults.end(),
		std::back_inserter( results ), ToSubstringRef( str )
	);
}


/**
 * @brief Split a given input string into a vector of substring references according to a predicate.
 *
 * # Abstract
 *
 * This method reads the @c input string and tokenizes it according to a custom @c splitPredicate.
 * For any character it returns true a new substring is formed from the last split point
 * (or the beginning of the string) to this character as a substring reference.
 * Since substring references are created, no data is copied, but the caller must
 * make sure that reference to the @c input string does not become invalid as long as
 * the substrings are processed further. The result substring references are stored in
 * a new vector which is created and returned by the function.
 *
 * # Usage
 *
 * @code
 *    std::string toSplit = "This is a test";
 *    // split the input strings when at spaces and return references to the substrings
 *    const auto& substrings = splitStringRef( toSplit, boost::is_space() );
 * @endcode
 *
 * @tparam String
 *    the string type used as input
 * @tparam SplitPredicate
 *    the predicate type used for deciding about a split for each character in the input string
 * @param[in] input
 *    reference to the string to be split
 * @param[in] splitPredicate
 *    the predicate that causes a split when returning true for a character in the input string
 * @return a vector with substring references into the input string representing the split results.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename String,
	typename SplitPredicate
>
std::vector< ns_types::SubstringRef< String > >
splitStringRef( String& input, SplitPredicate splitPredicate ) {
	std::vector< ns_types::SubstringRef< String > > substrings;
	splitStringRef( input, splitPredicate, substrings );
	return substrings;
}

} /* end namespace ns_algorithms */


#endif /* LIBCPP_ALGORITHMS_SPLITSTRINGREF_HPP_ */
