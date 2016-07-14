/*
 * SubstringRef.hpp
 *
 *  Created on: Apr 28, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_DETAIL_SUBSTRINGREF_HPP_
#define LIBCPP_TYPES_DETAIL_SUBSTRINGREF_HPP_

#include <functional>


namespace ns_types {

/**
 * @brief A lightweight reference to a substring of a @c std::string.
 *
 * @tparam String
 *    the underlying string implementation
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename String
>
class SubstringRef {
private:

	/// a string reference
	typedef std::reference_wrapper< String > StringRef;

public:

	/// a type representing the length of the underlying string
	typedef typename String::size_type StringLength;

	/// a type representing an offset in the underlying string
	typedef typename String::size_type StringOffset;

	/// a constant indicating the "until the end of the string"
	static const StringLength EndOfString;


	/**
	 * @brief Create a reference to a substring of an underlying string.
	 *
	 * @param[in] str
	 *    reference to the base string
	 * @param[in] startOffset
	 *    start offset of the substring (default 0)
	 * @param[in] length
	 *    length of the substring (default @c EndOfString)
	 */
	SubstringRef( String& str,
		const StringOffset startOffset = 0,
		const StringLength length = EndOfString ) :
			mStringRef( str ), mStart( startOffset ), mLength( length ) {
	}

	/// default copy construction
	SubstringRef( const SubstringRef& ) = default;


	/// default copy assignment
	SubstringRef& operator=( const SubstringRef& ) = default;


	/**
	 * @brief Get the starting position of the substring reference in the underlying string.
	 *
	 * @return the substring's starting position.
	 */
	StringOffset getStartOffset() const {
		return mStart;
	}

	/**
	 * @brief Get the length of the substring reference in the underlying string.
	 *
	 * @note This method returns the length of the string @c reference that was
	 *       specified when the reference was created. This might be longer than
	 *       the underlying string to specify a reference "to the end of the string".
	 *       If the length of the actual substring is requested, use @c SubstringRef::size() or
	 *       @c SubstringRef::getStringLength() to calculate the size without materializing the
	 *       substring reference into a new string instance.
	 *
	 * @return the substring reference's length.
	 */
	StringLength getLength() const {
		return mLength;
	}

	/**
	 * @brief Get the length of the resulting string when this instance is dereferenced.
	 *
	 * This method calculates the length of the substring that would be returned if
	 * this reference is dereferenced without materializing it.
	 *
	 * @return the length of the substring that would be constructed when this reference
	 *         is dereferenced at the current point in time (not thread-safe)
	 */
	StringLength getStringLength() const {
		StringLength length = mLength;

		if( mLength == EndOfString
			|| mStringRef.get().size() < mStart + mLength ) {
			length = mStringRef.get().size() - mStart;
		}

		return length;
	}

	/**
	 * @brief Get the length of the resulting string when this instance is dereferenced.
	 *
	 * @see SubstringRef::getStringLength()
	 *
	 * @return the length of the substring that would be constructed when this reference
	 *         is dereferenced at the current point in time (not thread-safe)
	 */
	inline StringLength size() const {
		return getStringLength();
	}

	/**
	 * @brief Return a new string instance as copy of the referenced substring.
	 *
	 * @return a new string instance having a copy of the referenced substring.
	 */
	String operator*() const {
		return mStringRef.get().substr( mStart, mLength );
	}


private:

	StringRef mStringRef; /**< the underlying string this reference refers to */
	StringOffset mStart;  /**< the starting position of the substring */
	StringLength mLength; /**< the length of the substring */
};


template< typename String >
const typename SubstringRef< String >::StringLength SubstringRef< String >::EndOfString = String::npos;

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_DETAIL_SUBSTRINGREF_HPP_ */
