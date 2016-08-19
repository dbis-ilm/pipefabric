#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include "cep/Matcher.hpp"
#include "cep/NFAController.hpp"
#include "cep/dsl/CEPExpr.hpp"

using namespace pfabric;

typedef Tuple<int, int, int> InTuple;
typedef TuplePtr<InTuple> InTuplePtr;
typedef Tuple<int, int, int> OutTuple;
typedef TuplePtr<OutTuple> OutTuplePtr;
typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;

TEST_CASE("Defining states and transitions using a DSL", "[CEP]") {
  auto matcher = std::make_shared<Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>>(
			Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::FirstMatch);

  Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::PredicateMap predicates = {
    {"A", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 1);
    }},
    {"B", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 2);
  	}},
    {"C", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 3);
  	}}
  };
  matcher->constructNFA(SEQ({ _S("A"), _S("B"), _S("C")}), predicates);

  matcher->constructNFA(SEQ({ _S("A"), OR({ _S("B"), _S("C") }), _S("D") }), {
    {"A", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 1);
    }},
    {"B", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 2);
  	}},
    {"C", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 3);
  	}},
    {"D", [&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
  		return (getAttribute<0>(*tp) == 4);
  	}}});
}
