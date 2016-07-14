/*
 * ErrorInfo.hpp
 *
 *  Created on: Apr 29, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_EXCEPTIONS_ERRORINFO_HPP_
#define LIBCPP_UTILITIES_EXCEPTIONS_ERRORINFO_HPP_

#include "libcpp/preprocessor/MacroEnd.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <boost/exception/info.hpp>
#include <boost/exception/detail/type_info.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/transformed.hpp>


namespace ns_utilities {
namespace exceptions {

/**
 * @brief A tagged vector for storing multiple diagnostic error information in an exception.
 *
 * The error information vector is tagged to be able to dispatch the formatting
 * function for custom types which is currently implemented in the @c operator<<().
 *
 * TODO Currently it doesn't work to override the formatting behavior for a custom type.
 *      Have a look how the boost framework handles this with respect to namespaces and fix this.
 *      Possibly provide a macro for this to generate boilerplate code for that.
 *
 * @tparam ErrorInfoTag
 *    a compile-time tag used by the boost exception framework for identifying the
 *    error information within an exception, can be an incomplete type
 * @tparam ErrorInfoType
 *    the actual type for a single error information
 */
template<
	typename ErrorInfoTag,
	typename ErrorInfoType
>
class ErrorInformationVec :
    public std::vector< ErrorInfoType > {
private:

	/// the base vector class which actually provides the implementation
    typedef std::vector< ErrorInfoType > Base;


public:
    /// inherit the base constructors
    using Base::Base;

    /**
     * @brief Formatting function for the error information contained in the vector.
     *
     * This method will print each error information stored in the vector in a separate line
     * using the contained type's @c operator<<().
     *
     * @param[in,out] out
     *    the output stream for formatting the diagnostic error information
     * @return a reference to the modified output stream
     */
    std::ostream& operator<<( std::ostream& out ) const {
        if( !this->empty() ) {
        	for( const auto& error: *this ) {
        		out << "\n\t" << error;
        	}
        }
        else {
            out << "------";
        }
        return out;
    }
};

/**
 * @brief Default formatting function for an @c ErrorInformationVec.
 *
 * This method can be used to format the diagnostic error information carried in an
 * @c ErrorInformationVec vector. This function will be resolved by the
 * @c boost::diagnostic_information() rendering function of the Boost.Exception framework.
 * This default implementation forwards to the @c operator<<() implementation
 * of the @c ErrorInformationVec.
 *
 * TODO provide a way for the user to implement custom formatting of generated error messages.
 *
 * @tparam ErrorInfoTag
 *    a compile-time tag used by the boost exception framework for identifying the
 *    error information within an exception, can be an incomplete type
 * @tparam ErrorInfoType
 *    the actual type for a single error information
 * @param[in] errors
 *    the error information vector
 * @param[in,out] out
 *    the output stream for formatting the diagnostic error information
 * @return a reference to the modified output stream
 */
template<
	typename ErrorInfoTag,
	typename ErrorInfoType
>
std::ostream& operator<<( std::ostream& out, const ErrorInformationVec< ErrorInfoTag, ErrorInfoType >& errors ) {
    return errors.operator<<( out );
}


/**
 * @brief An alias for a collection of diagnostic error information of a specific type.
 *
 * This type can be used to store multiple diagnostic error information of @c ErrorInfoType
 * in an exception instance under one variable. The infos are stored in a vector that
 * can be expanded using the @c ExceptionBase::operator<<() overload.
 * This can be useful if the number of error information is not known until runtime.
 * Storing the infos without the vector wrapper would override previously set information.
 *
 * @tparam ErrorInfoTag
 *    a compile-time tag used by the boost exception framework for identifying the
 *    error information within an exception, can be an incomplete type
 * @tparam ErrorInfoType
 *    the actual type for a single error information
 */
template< typename ErrorInfoTag, typename ErrorInfoType >
using ErrorInfo = boost::error_info< ErrorInfoTag, ErrorInfoType >;

/**
 * @brief An alias for a collection of diagnostic error information of a specific type.
 *
 * This type can be used to store multiple diagnostic error information of @c ErrorInfoType
 * in an exception instance under one variable. The infos are stored in a vector that
 * can be expanded using the @c operator<<.
 * This can be useful if the number of error information is not known until runtime.
 * Storing the infos without the vector wrapper would override previously set information.
 *
 * @tparam ErrorInfoTag
 *    a compile-time tag used by the boost exception framework for identifying the
 *    error information within an exception, can be an incomplete type
 * @tparam ErrorInfoType
 *    the actual type for a single error information
 */
template< typename ErrorInfoTag, typename ErrorInfoType >
using ErrorInfoCollection = boost::error_info<
	ErrorInfoTag,
	ErrorInformationVec< ErrorInfoTag, ErrorInfoType >
>;


/**
 * @brief A single piece of diagnostic error information of a specific type.
 *
 * This type stores a single piece of error information of @c ErrorInfoType.
 *
 * @tparam ErrorInfoTag
 *    a compile-time tag used by the boost exception framework for identifying the
 *    error information within an exception, can be an incomplete type
 * @tparam ErrorInfoType
 *    the actual type of the error information
 */
template<
	typename ErrorInfoTag,
	typename ErrorInfoType
>
using ErrorInfoCollectionEntry = boost::error_info<
	ErrorInfoTag,
	ErrorInfoType
>;


/**
 * @brief A tag wrapper structure to distinguish plain tagged error information entries
 *        from their wrapping container structures at compile-time.
 *
 * @tparam ErrorInfoTag
 *    the original tag used for the error information which is wrapped
 */
template< typename ErrorInfoTag >
struct CollectionOf {};


namespace impl {

/**
 * @brief A functor for wrapping a plain data type into an error information.
 *
 * A functor is required since lambdas cannot be used in a decltype expression.
 *
 * @tparam ErrorInfoType
 *    the target error information type
 */
template<
	typename ErrorInfoType
>
struct ToErrorInfo {
	/// the functor's result type
	typedef ErrorInfoType result_type;

	/**
	 * @brief The functor's call operator.
	 *
	 * This operator creates a new error information instance with invoking the
	 * @c ErrorInfoType constructor on the @c errorInfo to be wrapped.
	 *
	 * @param[in] errorInfo
	 *    the error information to be wrapped
	 * @return the wrapped error information
	 */
	template< typename ErrorInfo >
	result_type operator()( const ErrorInfo& errorInfo ) const {
		return ErrorInfoType( errorInfo );
	}
};

} /* end namespace impl */


/**
 * @brief A conversion function for wrapping a collection of data in an
 *        error information structure.
 *
 * This conversion functor will wrap a range of an @c errorInfos collection into a
 * collection of @c ErrorInfoType for collecting it as an exception's diagnostic
 * error information. The @c errorInfos iterator range from
 * [@c begin, ... , @c end] will be converted.
 *
 * @tparam ErrorInfoType
 *    the target error information type,
 *    must be explicitly specified
 * @tparam ErrorInfoIterator
 *    the iterator type used to construct the error information range
 * @param[in] begin
 *    the iterator for the first element (inclusive) in the range to be converted
 * @param[in] end
 *    the iterator for the last element (exclusive) in the range to be converted
 * @return an @c boost::iterator_range covering the wrapped error information entries
 */
template<
	typename ErrorInfoType,
	typename ErrorInfoIterator
>
auto toErrorInfos( const ErrorInfoIterator& begin, const ErrorInfoIterator& end ) ->
	decltype((
		boost::make_iterator_range( begin, end )
			| boost::adaptors::transformed( impl::ToErrorInfo< ErrorInfoType >() )
	))
{
	return ( boost::make_iterator_range( begin, end )
		| boost::adaptors::transformed( impl::ToErrorInfo< ErrorInfoType >() )
	);
}

/**
 * @brief A conversion function for wrapping a whole collection of data in an
 *        error information structure.
 *
 * This conversion functor will wrap the entire @c errorInfos collection into a
 * collection of @c ErrorInfoType for collecting it as an exception's diagnostic
 * error information. The whole @c errorInfos range from
 * [@c std::begin( errorInfos ), ... , @c std::end( errorInfos )] will be converted.
 *
 * @tparam ErrorInfoType
 *    the target error information type,
 *    must be explicitly specified
 * @tparam ErrorInfos
 *    the collection type used to store the error information to be converted,
 *    is automatically deduced
 * @param[in] errorInfos
 *    a collection of data to be wrapped, must support @c std::begin() and @c std::end()
 * @return an @c boost::iterator_range covering the wrapped error information entries
 */
template<
	typename ErrorInfoType,
	typename ErrorInfos
>
auto toErrorInfos( const ErrorInfos& errorInfos ) ->
	decltype((
		toErrorInfos< ErrorInfoType >( std::begin( errorInfos ), std::end( errorInfos ) )
	))
{
	return toErrorInfos< ErrorInfoType >( std::begin( errorInfos ), std::end( errorInfos ) );
}

} /* end namespace exceptions */
} /* end namespace ns_utilities */


/**
 * @brief A macro for declaring a new type for a single piece of error information in an exception.
 *
 * This macro defines a new @c errorInfoType under a @c error_info namespace in the current
 * namespace scope. This errorInfoType can be used in exceptions to collect a single diagnostic
 * error information of type @c infoType at runtime. Inserting it multiple times into an
 * exception will overwrite previous error information.
 */
#define DECLARE_ERROR_INFO( errorInfoType, infoType )                                          \
    namespace error_info {                                                                     \
        typedef ns_utilities::exceptions::ErrorInfo<                                           \
            struct tag##errorInfoType,                                                         \
            infoType                                                                           \
        > errorInfoType;                                                                       \
    }                                                                                          \
    MACRO_END

/**
 * @brief A macro for declaring a new type for collecting many error information in an exception.
 *
 * This macro defines a new @c errorInfoType under a @c error_info namespace in the current
 * namespace scope. This errorInfoType can be used in exceptions to collect many diagnostic
 * error information of type @c infoType at runtime. Inserting it multiple times into an
 * exception will append the the latest error information to previously specified ones.
 */
#define DECLARE_ERROR_INFO_COLLECTION( errorInfoType, infoType )                               \
    namespace error_info {                                                                     \
        typedef ns_utilities::exceptions::ErrorInfoCollectionEntry<                            \
        	ns_utilities::exceptions::CollectionOf< struct tag##errorInfoType >,               \
            infoType                                                                           \
        > errorInfoType;                                                                       \
    }                                                                                          \
    MACRO_END


#endif /* LIBCPP_UTILITIES_EXCEPTIONS_ERRORINFO_HPP_ */
