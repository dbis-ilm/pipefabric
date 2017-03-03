#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <string>

#include "qop/AggregateFunctions.hpp"
#include "core/Tuple.hpp"

using namespace pfabric;

TEST_CASE("Calculate sum of ints", "[AggregateFunc]") {
	AggrSum<int> aggr;
	int res = 0;
	for (int i = 0; i < 100; i++) {
		res += i;
		aggr.iterate(i);
	}
	REQUIRE(aggr.value() == res);

	aggr.init();
	REQUIRE(aggr.value() == 0);
}

TEST_CASE("Calculate moving sum of ints", "[AggregateFunc]") {
	AggrSum<int> aggr;
	int res = 0;
	for (int i = 0; i < 100; i++) {
		res += i;
		aggr.iterate(i);
	}
	for (int i = 1; i < 100; i += 10) {
		res -= i;
		aggr.iterate(i, true);
	}
	REQUIRE(aggr.value() == res);
}

TEST_CASE("Calculate count of ints", "[AggregateFunc]") {
	AggrCount<int, int> aggr;
	int res = 0;
	for (int i = 0; i < 100; i++) {
		res ++;
		aggr.iterate(i);
	}
	REQUIRE(aggr.value() == res);

	aggr.init();
	REQUIRE(aggr.value() == 0);
}

TEST_CASE("Calculate moving count of ints", "[AggregateFunc]") {
	AggrCount<int, int> aggr;
	int res = 0;
	for (int i = 0; i < 100; i++) {
		res++;
		aggr.iterate(i);
	}
	for (int i = 1; i < 100; i += 10) {
		res --;
		aggr.iterate(i, true);
	}
	REQUIRE(aggr.value() == res);
}

TEST_CASE("Calculate count of strings", "[AggregateFunc]") {
	AggrCount<std::string, int> aggr;
	int res = 0;
	for (int i = 0; i < 100; i++) {
		res++;
		std::stringstream strm;
		strm << "String#" << i;
		std::string s = strm.str();
		aggr.iterate(s);
	}
	REQUIRE(aggr.value() == res);
}

TEST_CASE("Calculate average of ints", "[AggregateFunc]") {
	AggrAvg<int, double> aggr;
	int res = 0;
	for (int i = 0; i < 100; i++) {
		res += i;
		aggr.iterate(i);
	}
	REQUIRE(aggr.value() == res / 100.0);
}

TEST_CASE("Calculate moving average of ints", "[AggregateFunc]") {
	AggrAvg<int, double> aggr;
	int res = 0, num = 0;
	for (int i = 0; i < 100; i++) {
		res += i; num++;
		aggr.iterate(i);
	}
	for (int i = 1; i < 100; i += 10) {
		res -= i; num--;
		aggr.iterate(i, true);
	}
	REQUIRE(aggr.value() == res / (double)num);
}

TEST_CASE("Calculate global minimum of ints", "[AggregateFunc]") {
	AggrGlobalMin<int> aggr;
	for (int i = 0; i < 100; i++) {
		aggr.iterate(i);
	}
	REQUIRE(aggr.value() == 0);
}

TEST_CASE("Calculate global minimum of strings", "[AggregateFunc]") {
	std::vector<std::string> data = { "aaa", "bbb", "ccc", "ddd" };

	AggrGlobalMin<std::string> aggr;
	for (unsigned i = 0; i < data.size(); i++) {
		aggr.iterate(data[i]);
	}
	REQUIRE(aggr.value() == "aaa");
}

TEST_CASE("Calculate global maximum of ints", "[AggregateFunc]") {
	AggrGlobalMax<int> aggr;
	for (int i = 0; i < 100; i++) {
		aggr.iterate(i);
	}
	aggr.iterate(5);
	REQUIRE(aggr.value() == 99);
}

TEST_CASE("Calculate global maximum of strings", "[AggregateFunc]") {
	std::vector<std::string> data = { "aaa", "bbb", "ccc", "ddd", "aa" };

	AggrGlobalMax<std::string> aggr;
	for (unsigned i = 0; i < data.size(); i++) {
		aggr.iterate(data[i]);
	}
	REQUIRE(aggr.value() == "ddd");
}

TEST_CASE("Calculate min/max of ints", "[AggregateFunc]") {
	AggrMinMax<int, std::less<int>> aggr1;
	AggrMinMax<int, std::greater<int>> aggr2;
	for (int i = 0; i < 100; i++) {
		aggr1.iterate(i);
		aggr2.iterate(i);
	}
	REQUIRE(aggr1.value() == 0);
	REQUIRE(aggr2.value() == 99);
}

TEST_CASE("Calculate least and most recent values", "[AggregateFunc]") {
	AggrLRecent<int> aggr1;
	AggrMRecent<int> aggr2;
	for (int i = 0; i < 100; i++) {
		aggr1.iterate(i);
		aggr2.iterate(i);
	}
	REQUIRE(aggr1.value() == 0);
	REQUIRE(aggr2.value() == 99);
}

TEST_CASE("Calculate most recent values with timestamps", "[AggregateFunc]") {
	AggrMRecent<int> aggr;
	Timestamp ts;
	for (int i = 0; i < 100; i++) {
		ts = (i < 50 ? i : 100 - i);
		aggr.iterate(i, ts);
	}
	REQUIRE(aggr.value() == 50);
}

TEST_CASE("Calculate distinct count", "[AggregateFunc]") {
	AggrDCount<double, double> dcount;
	for (int i = 0; i < 100; i++) {
		dcount.iterate(i);
	}
	for (int i = 0; i < 100; i += 2) {
		dcount.iterate(i);
	}
	for (int i = 90; i < 120; i += 2) {
		dcount.iterate(i);
	}

	REQUIRE(dcount.value() == 110);
}


TEST_CASE("Test AggrIdentity function", "[AggregateFunc]") {
  AggrIdentity<int> aggr1;

  for (int i = 0; i < 10; i++)
    aggr1.iterate(i);

  REQUIRE(aggr1.value() == 9);

  /* ------------------------------- */

  AggrIdentity<std::string> aggr2;
	std::vector<std::string> data = { "aaa", "bbb", "ccc", "ddd", "eee" };

  for (int i = 0; i < data.size(); i++)
    aggr2.iterate(data[i]);

  REQUIRE(aggr2.value() == "eee");
}
