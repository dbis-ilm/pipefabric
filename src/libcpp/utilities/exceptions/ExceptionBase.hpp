/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/*
 * ExceptionBase.hpp
 *
 *  Created on: Apr 29, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_EXCEPTIONBASE_HPP_
#define LIBCPP_UTILITIES_EXCEPTIONBASE_HPP_

#include "ErrorInfo.hpp"

#include <exception>
#include <boost/exception/all.hpp>
#include <boost/range/algorithm/for_each.hpp>


namespace ns_utilities {
namespace exceptions {

/**
 * @brief Base class for exception handling.
 *
 * This class is the base class for exceptions. It implements the standard
 * exception interface as well as the boost version to allow to inject more
 * detailed error information.
 *
 * In addition to the @c boost::exception implementation, it allows to collect
 * many diagnostic error information entries at runtime that would be overwritten
 * in the original implementation as soon as a new information is inserted
 * into the exception via the @c operator<<().
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
struct ExceptionBase :
	public virtual std::exception,
	public virtual boost::exception
{};

/**
 * @brief A default field that can be used in the exception to provide a
 *        detailed error description consisting of multiple lines as strings.
 */
DECLARE_ERROR_INFO_COLLECTION( Description, std::string );

} /* end namepace exceptions */
} /* end namespace ns_utilities */


/////////////  adapt the new exception for the boost error info operators  /////////////

namespace boost {
namespace exception_detail {

/**
 * @brief Specialization for a collection of error information.
 *
 * This method collects a new piece of error information generated at runtime
 * in the exceptions diagnostic error information provided by the boost
 * exception base class. It prevents that existing error information is
 * overwritten with wrapping the information into a vector and appending
 * the new entry at the end.
 *
 * @tparam Exception
 *    the exception class that collects the error information
 * @tparam ErrorInfoTag
 *    the compile-time tag for the type of error information that is collected
 * @tparam ErrorInfoType
 *    the actual type of the error information to be collected
 * @param[in] errorInfo
 *    the error description
 * @return a reference to the exception
 */
template<
	typename Exception,
	typename ErrorInfoTag,
	typename ErrorInfoType
>
const Exception& set_info(
		const Exception& exception,
		const ns_utilities::exceptions::ErrorInfoCollectionEntry<
			ns_utilities::exceptions::CollectionOf< ErrorInfoTag >,
			ErrorInfoType
		>& errorInfo
	)
{
	using namespace ns_utilities::exceptions;
    typedef ErrorInfoCollection< ErrorInfoTag, ErrorInfoType > ErrorCollection;
    typedef ErrorInformationVec< ErrorInfoTag, ErrorInfoType > ErrorInformation;

	auto* errorInfos = boost::get_error_info< ErrorCollection >(
		// the error information vector must be modified -> remove const qualifier
		const_cast< Exception& >( exception )
	);

	if( errorInfos != nullptr ) {
		// the wrapping error information vector does already exists
		// --> just append the new piece of information
		errorInfos->push_back( errorInfo.value() );
	}
	else {
		// no error information for the requested tag has been provided yet
		// --> create a new vector, append the entry and insert it in the exception
		//     using the original tag as key
		ErrorInformation newErrorInfos;
		newErrorInfos.push_back( errorInfo.value() );
		set_info( exception, ErrorCollection( newErrorInfos ) );
	}

	return exception;
}

/**
 * @brief Specialization for a whole collection of error information.
 *
 * This method collects all error information entries covered in a range in the
 * exception's diagnostic information. Use the @c exceptions::ns_utilities::toErrorInfos()
 * conversion function for generating such a range from a container.
 *
 * @tparam Exception
 *    the exception class that collects the error information
 * @tparam ErrorInfoTag
 *    the compile-time tag for the type of error information that is collected
 * @tparam ErrorInfoType
 *    the actual type of the error information to be collected
 * @param[in] errorInfo
 *    the error description
 * @return a reference to the exception
 */
template<
	typename Exception,
	typename Iterator
>
const Exception& set_info( const Exception& exception,
		const boost::iterator_range< Iterator >& errors
	)
{
	typedef typename boost::iterator_range< Iterator >::value_type ErrorInfo;

	// invoke the set_info function for each element in the error info range
	boost::range::for_each( errors,
		[&]( const ErrorInfo& error ) {
			set_info( exception, error );
		}
	);

	return exception;
}

} /* end namespace exception_detail */

/**
 * @brief Output stream operator overload for a collection of error information.
 *
 * This method will inject a whole collection of error information into
 * an exception's error info structure. Use the @c exceptions::ns_utilities::toErrorInfos()
 * conversion function for generating such a range from a container.
 *
 * @param[in,out] exception
 *    the exception collecting the error information
 * @param[in] errors
 *    the range of error information to be injected into the exception
 * @return a reference to the exception
 */
template<
	typename Exception,
	typename Iterator
>
inline
typename enable_if< exception_detail::derives_boost_exception<Exception>, const Exception& >::type
operator<<( const Exception& exception, const boost::iterator_range< Iterator >& errors ) {
	return exception_detail::set_info( exception, errors );
}

/**
 * @brief Output stream operator overload for an error information collection entry.
 *
 * This method be used if a single entry of an error information collection
 * is inserted into an exception.
 *
 * @param[in,out] exception
 *    the exception collecting the error information
 * @param[in] errorInfo
 *    the error information entry to be injected into the exception
 * @return a reference to the exception
 */
template<
	typename Exception,
	typename ErrorInfoTag,
	typename ErrorInfoType
>
inline
typename enable_if< exception_detail::derives_boost_exception<Exception>, const Exception& >::type
operator<<( const Exception& exception,
	const ns_utilities::exceptions::ErrorInfoCollectionEntry<
		ns_utilities::exceptions::CollectionOf< ErrorInfoTag >,
		ErrorInfoType
	>& errorInfo ) {
	return exception_detail::set_info( exception, errorInfo );
}

} /* end namespace boost */

#endif /* LIBCPP_UTILITIES_EXCEPTIONBASE_HPP_ */
