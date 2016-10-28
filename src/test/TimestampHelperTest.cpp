#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

#include "pfabric.hpp"

using namespace pfabric;

/**
 *
 * Test cases for the TimestampHelper functions.
 */

/**
 * Test #1: Parse a timestamp from a string and convert it to a ptime value.
 */
TEST_CASE( "Parse a Timestamp from a string", "[Timestamp]" ) {
  // TODO: this is only a hack for GMT+1 DST!
  Timestamp t1 = TimestampHelper::stringToTimestamp("2012-10-20T17:14:27.841+0000");

  boost::posix_time::ptime pt (boost::posix_time::time_from_string("2012-10-20 15:14:27.841"));
  Timestamp t2 = TimestampHelper::timestampFromTime(pt);
  REQUIRE(t1 == t2);

  std::string ts = TimestampHelper::timestampToString(t2);
  REQUIRE(ts == "2012-Oct-20 15:14:27.841000");
}
