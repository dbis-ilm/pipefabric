/*
 * TimeStampHelper.cpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#include "TimestampHelper.hpp"

#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>


namespace pfabric {

boost::posix_time::ptime UNIX_EPOCH( boost::gregorian::date(1970,1,1) );

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
