/*
 * PrintCSV.hpp
 *
 *  Created on: Jun 5, 2014
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_PRINTCSV_HPP_
#define LIBCPP_UTILITIES_PRINTCSV_HPP_

#include <ostream>
#include <string>
#include <cassert>


namespace ns_utilities {

/**
 * @brief Functor for printing a series of elements in CSV format.
 *
 * @tparam NumElements
 *           the total number of elements that are contained in the series
 */
template< unsigned int NumElements >
struct PrintCSV {

	/**
	 * @brief Create a new CSV printer.
	 *
	 * @param[in] target
	 *               the output stream for printing the data to
	 * @param[in] beginDelim
	 *               the delimiter that is prepended before the separated values
	 *               (default "")
	 * @param[in] endDelim
	 *               the delimiter that is appended after the series has been printed
	 *               (default "")
	 * @param[in] valSep
	 *               the separator between the values
	 *               (default ",")
	 */
	PrintCSV( std::ostream& target,
			const std::string& beginDelim = std::string(),
			const std::string& endDelim = std::string(),
			const std::string& valSep = std::string( "," ) )
		: mTarget( target ), mValueSeparator( valSep ), mBeginDelimiter( beginDelim ),
		  mEndDelimiter( endDelim ), mValuesPrinted( 0 ) {}

	/**
	 * @brief The function that is invoked for each element in the sequence.
	 *
	 * If the function is invoked the first time, the begin delimiter is printed
	 * before the actual element value. If it is invoked the last time, i.e.,
	 * the number of elements is reached, the end delimiter is appended.
	 *
	 * @param[in] element
	 *               the value of the element that shall be printed
	 * @tparam Element
	 *           the type of the element that shall be printed
	 */
	template< typename Element >
	void operator() ( const Element& element ) const {
		assert( mValuesPrinted < NumElements );

		if( mValuesPrinted == 0 ) {
			mTarget << mBeginDelimiter;
		}

		mTarget << element;
		mValuesPrinted++;

		mTarget << (( mValuesPrinted < NumElements ) ? mValueSeparator : mEndDelimiter) ;
	}

	std::ostream& mTarget;             /**< the output stream for printing the values */
	const std::string mValueSeparator; /**< the separator between the values */
	const std::string mBeginDelimiter; /**< the delimiter before all values */
	const std::string mEndDelimiter;   /**< the delimiter after all values */
	mutable std::size_t mValuesPrinted;/**< internal counter for printed values */
};


} /* end namespace ns_utilities */


#endif /* LIBCPP_UTILITIES_PRINTCSV_HPP_ */
