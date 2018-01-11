/*
 * Copyright (c) 2014-18 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/Map.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/TextFileSource.hpp"
#include "qop/TupleExtractor.hpp"
#include "qop/JsonExtractor.hpp"
#include "qop/TupleDeserializer.hpp"
#include "StreamMockup.hpp"

using namespace pfabric;

typedef TuplePtr< int, int, int > ATuplePtr;

TEST_CASE("Extracting tuples from text lines", "[TupleExtractor]") {
	const char *s[] = { "0,0,0", "1,1,10", "2,2,20" };

	std::vector<TStringPtr> input = {
		makeTuplePtr(StringRef(s[0], strlen(s[0]))),
		makeTuplePtr(StringRef(s[1], strlen(s[1]))),
		makeTuplePtr(StringRef(s[2], strlen(s[2]))) };

	std::vector<ATuplePtr> expected = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr(1, 1, 10),
		makeTuplePtr(2, 2, 20) };

	auto mockup = std::make_shared< StreamMockup<TStringPtr, ATuplePtr> >(input, expected);

	auto extractor = std::make_shared< TupleExtractor<ATuplePtr> >();

	CREATE_DATA_LINK(mockup, extractor)
	CREATE_DATA_LINK(extractor, mockup)

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

TEST_CASE("Extracting tuples from text lines with a non-standard separator", "[TupleExtractor]") {
	const char *s[] = { "0|0|0", "1|1|10", "2|2|20" };

	std::vector<TStringPtr> input = {
		makeTuplePtr(StringRef(s[0], strlen(s[0]))),
		makeTuplePtr(StringRef(s[1], strlen(s[1]))),
		makeTuplePtr(StringRef(s[2], strlen(s[2]))) };

	std::vector<ATuplePtr> expected = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr(1, 1, 10),
		makeTuplePtr(2, 2, 20) };

	auto mockup = std::make_shared< StreamMockup<TStringPtr, ATuplePtr> >(input, expected);

	auto extractor = std::make_shared< TupleExtractor<ATuplePtr> >('|');

	CREATE_DATA_LINK(mockup, extractor)
	CREATE_DATA_LINK(extractor, mockup)

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

TEST_CASE("Extracting tuples from text lines with null values", "[TupleExtractor]") {
	const char *s[] = { "0|0|", "1||10", "||20" };

	std::vector<TStringPtr> input = {
		makeTuplePtr(StringRef(s[0], strlen(s[0]))),
		makeTuplePtr(StringRef(s[1], strlen(s[1]))),
		makeTuplePtr(StringRef(s[2], strlen(s[2]))) };

	std::vector<ATuplePtr> expected = {
		makeTuplePtr(0, 0, 0),
		makeTuplePtr(1, 0, 10),
		makeTuplePtr(0, 0, 20) };

	expected[0]->setNull(2); expected[1]->setNull(1);
	expected[2]->setNull(0); expected[2]->setNull(1);

	auto mockup = std::make_shared< StreamMockup<TStringPtr, ATuplePtr> >(input, expected);

	auto extractor = std::make_shared< TupleExtractor<ATuplePtr> >('|');

	CREATE_DATA_LINK(mockup, extractor)
	CREATE_DATA_LINK(extractor, mockup)

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

TEST_CASE("Extracting more tuples from text lines with null values", "[TupleExtractor]") {
	const char *s[] = {
		"2010-03-03T17:34:40.328+0000|405979|974|LOL|Heinz Frank|405972|",
		"2010-03-03T17:35:33.552+0000|406918|974|great|Heinz Frank|406917|" };

	std::vector<TStringPtr> input = {
			makeTuplePtr(StringRef(s[0], strlen(s[0]))),
			makeTuplePtr(StringRef(s[1], strlen(s[1]))) };

	typedef TuplePtr<std::string, long, long, std::string, std::string, long, long> CommentType;

	std::vector<CommentType> expected = {
		makeTuplePtr(std::string("2010-03-03T17:34:40.328+0000"),
			405979l, 974l, std::string("LOL"), std::string("Heinz Frank"), 405972l, 0l),
		makeTuplePtr(std::string("2010-03-03T17:35:33.552+0000"),
			406918l, 974l, std::string("great"), std::string("Heinz Frank"), 406917l, 0l) };
		expected[0]->setNull(6); expected[1]->setNull(6);

	auto mockup = std::make_shared< StreamMockup<TStringPtr, CommentType> >(input, expected);

	auto extractor = std::make_shared< TupleExtractor<CommentType> >('|');

	CREATE_DATA_LINK(mockup, extractor)
	CREATE_DATA_LINK(extractor, mockup)

	mockup->start();

	REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

TEST_CASE("Extracting tuples from JSON strings", "[JsonExtractor]") {
	const char *s[] = { "{ \"key1\": 0, \"key3\": 101, \"key2\": 10 }",
		"{ \"key1\": 1, \"key2\": 11, \"key3\": 201 }",
		"{ \"key1\": 2, \"key3\": 301, \"key2\": 12 }" };

	std::vector<TStringPtr> input = {
		makeTuplePtr(StringRef(s[0], strlen(s[0]))),
		makeTuplePtr(StringRef(s[1], strlen(s[1]))),
		makeTuplePtr(StringRef(s[2], strlen(s[2]))) };

	std::vector<ATuplePtr> expected = {
		makeTuplePtr(0, 10, 101),
		makeTuplePtr(1, 11, 201),
		makeTuplePtr(2, 12, 301) };

	auto mockup = std::make_shared< StreamMockup<TStringPtr, ATuplePtr> >(input, expected);

	std::vector<std::string> keys { "key1", "key2", "key3" };
	auto extractor = std::make_shared<JsonExtractor<ATuplePtr>>(keys);

	CREATE_DATA_LINK(mockup, extractor)
	CREATE_DATA_LINK(extractor, mockup)

	mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}

TEST_CASE("Deserializing tuples from buffer", "[TupleDeserializer]") {
  std::vector<TBufPtr> input;

  std::vector<ATuplePtr> expected = {
    makeTuplePtr(0, 0, 0),
    makeTuplePtr(1, 0, 10),
    makeTuplePtr(0, 0, 20) };

  for (ATuplePtr& tp : expected) {
    StreamType res;
    tp->serializeToStream(res);
    input.push_back(makeTuplePtr((const StreamType&) res));
  }

  auto mockup = std::make_shared< StreamMockup<TBufPtr, ATuplePtr> >(input, expected);

  auto deserializer = std::make_shared< TupleDeserializer<ATuplePtr> >();

  CREATE_DATA_LINK(mockup, deserializer)
  CREATE_DATA_LINK(deserializer, mockup)

  mockup->start();

  REQUIRE(mockup->numTuplesProcessed() == expected.size());
}
