/*
 * ConvertTimeDurationToDouble.hpp
 *
 *  Created on: Jan 7, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_CONVERTTIMEDURATIONTODOUBLE_HPP_
#define LIBCPP_UTILITIES_CONVERTTIMEDURATIONTODOUBLE_HPP_

#include <chrono>


namespace ns_utilities {

/**
 * @brief Convert the current timer value into a target duration as floating point value.
 *
 * This method converts the current timer value into a target duration without truncating its
 * value when the target's accuracy is lower.
 *
 * @tparam SourceDuration
 *    the granularity of the source duration
 * @tparam TargetDuration
 *    the granularity of the target duration for the calculating the ratio
 * @param[in] value
 *    the duration value to be converted into a ratio
 * @return the converted ratio
 */
template< typename SourceDuration, typename TargetDuration >
double convertTimeDurationToDouble( const SourceDuration& value ) {
	typedef std::ratio_divide<
		std::chrono::nanoseconds::period,
		typename TargetDuration::period
	> ConversionRatio;
	const auto nanos = std::chrono::duration_cast< std::chrono::nanoseconds >( value );
	return static_cast<double>(nanos.count()) * ConversionRatio::num / ConversionRatio::den;
}

} /* end namespace ns_utilities */

#endif /* LIBCPP_UTILITIES_CONVERTTIMEDURATIONTODOUBLE_HPP_ */
