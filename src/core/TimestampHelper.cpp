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

#include "TimestampHelper.hpp"

#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>


namespace pfabric {

boost::posix_time::ptime UNIX_EPOCH( boost::gregorian::date(1970,1,1) );

/**
 * Converts the given string with a format (%Y-%m-%dT%H:%M:%S) into a timestamp.
 */
Timestamp TimestampHelper::stringToTimestamp(const std::string& date) {
	//TODO: make generic date expression
	//TODO: find UTC difference automatically, -1 for Germany
	struct std::tm tm;
	if( strptime(date.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL ) {
			/* TODO Handle error */
	}
	// TODO: strptime ignores the milliseconds:
	auto offs1 = date.find('.') + 1;
	auto offs2 = date.find('+', offs1);
	unsigned long milliseconds = std::stoi(date.substr(offs1, offs2 - offs1));
	//tm.tm_isdst = -1;   // to determine whether daylight saving time is considered
	// Timestamp t = time_to_epoch(&tm, -1); //faster than mktime but wrong!
	Timestamp t = timegm(&tm);
	return t * 1000000 + milliseconds * 1000;
}

Timestamp TimestampHelper::parseTimestamp(const std::string& val) {
	enum TimeFormat {
		Unknown,
		Unix,
		String,
		ISOString
	} timetype = Unknown;

	boost::regex unix_expr("^[0-9]+$");
	boost::regex string_expr("^[0-9]{4}-[0-1][0-9]-[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9](.[0-9]{3})?$");
	boost::regex iso_string_expr_secs("^[0-9]{8}T[0-9]{6}$");
	boost::regex iso_string_expr_msecs("^[0-9]{8}T[0-9]{6}.[0-9]{6}$");

	Timestamp result = 0;

	if (timetype == Unknown) {
		if (regex_match(val.c_str(), unix_expr)) {
			timetype = Unix;
		}
		else if (regex_match(val.c_str(), string_expr))	{
			timetype = String;
		}
		else if (regex_match(val.c_str(), iso_string_expr_secs) || regex_match(val.c_str(), iso_string_expr_msecs))	{
			timetype = ISOString;
		}
		else {
			BOOST_LOG_TRIVIAL(warning) << "could not identify timestamp type of \"" << val << "\". use internal.";
		}
	}
	switch(timetype) {
	case Unix:
		result = atol(val.c_str()) * 1000; // NOTE: we assume milliseconds here!!!
		break;
	case String:
		result = timestampFromTime(boost::posix_time::time_from_string(val));
		break;
	case ISOString:
		result = timestampFromTime(boost::posix_time::from_iso_string(val));
		break;
	case Unknown:
		try {
			std::stringstream s;
			s.exceptions (std::ios_base::failbit);
			s.str(val);
			s >> result;
		}
		catch (std::exception& e) {
			BOOST_LOG_TRIVIAL(warning) << "unable to parse timestamp \"" << val << "\":\n" << e.what();
		}
		catch ( ... ) {
			BOOST_LOG_TRIVIAL(warning) << "unable to parse timestamp \"" << val << "\". use internal.";
		}
		break;
	}
	return result;
}

} /* end namespace pquery */
