/*
 * TimestampHelper.hpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#ifndef TIMESTAMPHELPER_HPP_
#define TIMESTAMPHELPER_HPP_

#include "PFabricTypes.hpp"

#include <ctime>
#include <chrono>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace pfabric {

extern boost::posix_time::ptime UNIX_EPOCH;


/**
 * @brief A class for timestamp associated with each tuple.
 *
 * A timestamp represents the number of microseconds since 01/01/1970 indicating when the
 * tuple arrived in the system.
 */
struct TimestampHelper {

	/**
	 * Returns the current system time as timestamp (microseconds since 01/01/1970).
	 */
	static inline Timestamp timestampFromCurrentTime() {
		return (boost::posix_time::microsec_clock::local_time() - UNIX_EPOCH).total_microseconds();
	}

	/**
	 * Converts the given POSIX time into a timestamp (microseconds since 01/01/1970).
	 */
	static inline Timestamp timestampFromTime(const boost::posix_time::ptime& tm) {
		return (tm - UNIX_EPOCH).total_microseconds();
	}

	/**
	 * Parses the given string and tries to convert it into a timestamp.
	 */
	static Timestamp parseTimestamp(const std::string& ts);

	/**
	 * Returns the given timestamp as a POSIX time value.
	 */
	static inline boost::posix_time::ptime timestampToPtime(Timestamp ts) {
		return boost::posix_time::ptime(
			boost::gregorian::date(1970,1,1),
			boost::posix_time::time_duration(0, 0, 0, ts)
		);
	}

	/**
	 * Converts the given string with a format (%Y-%m-%d %H:%M:%S) into a timestamp (# of seconds since 01/01/1970).
	 */
	static inline Timestamp stringToTimestamp(const std::string& date) {
		//TODO: make generic date expression
		//TODO: find UTC difference automatically, -1 for Germany
		struct std::tm tm ;
		if( strptime(date.c_str(), "%Y-%m-%d %H:%M:%S", &tm) == NULL ) {
		    /* TODO Handle error */
		}

		//tm.tm_isdst = -1;   // to determine whether daylight saving time is considered
		Timestamp t = time_to_epoch(&tm, -1); //faster than mktime
		//Timestamp t = mktime(&tm);
		return t * 1000000;
	}


	/**
	 * TODO
	 *
	 * taken from https://gmbabar.wordpress.com/2010/12/01/mktime-slow-use-custom-function/
	 *
	 * @param ltm
	 * @param utcdiff
	 * @return
	 */
	static inline std::time_t time_to_epoch( const struct std::tm* ltm, int utcdiff ) {
		//TODO: If you sum the days in mon_days[], you can save the for() loop
//		static const unsigned int mon_days [] =           {31, 28, 31,  30,  31,  30,  31,  31,  30,  31,  30,  31};
		static const unsigned int prefixSumMonthDays [] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
		static const unsigned int SECONDS_PER_MIN  =  60;
		static const unsigned int SECONDS_PER_HOUR =  60 * SECONDS_PER_MIN;
		static const unsigned int SECONDS_PER_DAY = 365 * SECONDS_PER_HOUR;
		long tyears, tdays, leaps, utc_hrs;

		// TODO explain what happens here and when this is valid...

		tyears = ltm->tm_year - 70 ; // tm->tm_year is from 1900.
		leaps = (tyears + 2) / 4; // no of next two lines until year 2100.
		//i = (ltm->tm_year ‚Äì 100) / 100;
		//leaps -= ( (i/4)*3 + i%4 );

//		tdays = 0;
//		for( int month = 0; month < ltm->tm_mon; month++ ) {
//			tdays += mon_days[ month ];
//		}
		tdays = prefixSumMonthDays[ ltm->tm_mon ];

		tdays += ltm->tm_mday-1; // days of month passed.
		tdays = tdays + (tyears * 365) + leaps;

		utc_hrs = ltm->tm_hour + utcdiff; // for your time zone.
		return (tdays * SECONDS_PER_DAY)
			+ (utc_hrs * SECONDS_PER_HOUR)
			+ (ltm->tm_min * SECONDS_PER_MIN)
			+ ltm->tm_sec;
	}
};

} /* end namespace pquery */


#endif /* TIMESTAMPHELPER_HPP_ */
