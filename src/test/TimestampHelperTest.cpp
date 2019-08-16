/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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
  // Timestamp t1 = TimestampHelper::stringToTimestamp("1970-01-01T00:01:10.000+0000");
 Timestamp t1 = TimestampHelper::stringToTimestamp("2012-10-20T15:14:27.841+0000");

  //boost::posix_time::ptime pt (boost::posix_time::time_from_string("1970-01-01 00:01:10.000"));
  boost::posix_time::ptime pt (boost::posix_time::time_from_string("2012-10-20 15:14:27.841"));
  Timestamp t2 = TimestampHelper::timestampFromTime(pt);
  REQUIRE(t1 == t2);

  std::string ts = TimestampHelper::timestampToString(t2);
  REQUIRE(ts == "2012-Oct-20 15:14:27.841000");
}
