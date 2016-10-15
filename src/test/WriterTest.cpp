#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <boost/filesystem.hpp>

#include "core/Tuple.hpp"
#include "qop/ConsoleWriter.hpp"
#include "qop/FileWriter.hpp"

#include "StreamMockup.hpp"

#include "fmt/format.h"

using namespace pfabric;

typedef Tuple< int, int, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the stream_writer operator.
 */
TEST_CASE("Writing a tuple stream to console", "[ConsoleWriter]") {
	typedef ConsoleWriter< MyTuplePtr > TestWriter;
	std::vector<MyTuplePtr> input = {
		makeTuplePtr(0, 0, 22),
		makeTuplePtr (1, 1, 22),
		makeTuplePtr(2, 2, 22),
		makeTuplePtr(3, 3, 22),
		makeTuplePtr(4, 4, 22)
	};
		std::vector<MyTuplePtr> output;

	std::string expected = "0--22|0\n1--22|1\n2--22|2\n3--22|3\n4--22|4\n";
	std::stringstream strm;
	auto tgen = std::make_shared<StreamMockup<MyTuplePtr, MyTuplePtr> >(input, output);
	auto formatter = [&](std::ostream& os, const MyTuplePtr& tp ) {
		os << fmt::format("{0}--{1}|{0}", tp->getAttribute<0>(), tp->getAttribute<2>()) << std::endl;
	};
	auto writer = std::make_shared< TestWriter >(strm, formatter);
	CREATE_DATA_LINK(tgen, writer);

	tgen->start();
	REQUIRE(strm.str() == expected);
}

TEST_CASE("Writing a tuple stream to a file", "[FileWriter]") {
	typedef FileWriter< MyTuplePtr > TestWriter;
	std::vector<MyTuplePtr> input = {
		makeTuplePtr(0, 0, 22),
		makeTuplePtr (1, 1, 22),
		makeTuplePtr(2, 2, 22),
		makeTuplePtr(3, 3, 22),
		makeTuplePtr(4, 4, 22)
	};
		std::vector<MyTuplePtr> output;

	std::string expected = "0--22|0\n1--22|1\n2--22|2\n3--22|3\n4--22|4\n";
	auto tgen = std::make_shared<StreamMockup<MyTuplePtr, MyTuplePtr> >(input, output);
	auto formatter = [&](std::ostream& os, const MyTuplePtr& tp ) {
		os << fmt::format("{0}--{1}|{0}", tp->getAttribute<0>(), tp->getAttribute<2>()) << std::endl;
	};
	auto writer = std::make_shared< TestWriter >("test.dat", formatter);
	CREATE_DATA_LINK(tgen, writer);

	tgen->start();


	std::ifstream ifs("test.dat");
	std::string str((std::istreambuf_iterator<char>(ifs)),
	                 std::istreambuf_iterator<char>());

	REQUIRE(str == expected);
	boost::filesystem::remove("test.dat");
}
