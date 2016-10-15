#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

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

typedef Tuple< int, int, int > ATuple;
typedef TuplePtr< ATuple > ATuplePtr;

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

	auto mockup = std::make_shared< StreamMockup<TStringPtr, ATuplePtr> >(input, expected);

	auto extractor = std::make_shared< TupleExtractor<ATuplePtr> >('|');

	CREATE_DATA_LINK(mockup, extractor)
	CREATE_DATA_LINK(extractor, mockup)

	mockup->start();
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
}
