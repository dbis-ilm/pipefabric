#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/assign/std/vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "cep/Matcher.hpp"
#include "cep/NFAController.hpp"

#include "StreamMockup.hpp"

using namespace std;
using namespace boost::assign;
using namespace pfabric;

typedef Tuple<int, int, int> InTuple;
typedef TuplePtr<InTuple> InTuplePtr;
typedef Tuple<int, int, int> OutTuple;
typedef TuplePtr<OutTuple> OutTuplePtr;

TEST_CASE("Verifying the correct behavior of the CEP operator", "[CEP]") {
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >("cep_test.in", "cep_test.res");

	auto stateAFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 1; };
	auto stateBFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 2; };
	auto stateCFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 3; };

	auto matcher = std::make_shared<Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>>(
			Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::FirstMatch);

	auto nfa = matcher->getNFAController();

	auto edgeAState = nfa->createForwardEdge(stateAFilter);
	auto edgeBState = nfa->createForwardEdge(stateBFilter);
	auto edgeCState = nfa->createForwardEdge(stateCFilter);

	auto startStateA = nfa->createStartState("A");
	auto stateB = nfa->createNormalState("B");
	auto stateC = nfa->createNormalState("C");
	auto stateD = nfa->createFinalState("D");

	nfa->createForwardTransition(startStateA, edgeAState, stateB);
	nfa->createForwardTransition(stateB, edgeBState, stateC);
	nfa->createForwardTransition(stateC, edgeCState, stateD);

	CREATE_DATA_LINK(mockup, matcher);
	CREATE_DATA_LINK(matcher, mockup);

	mockup->start();
}

#if 1
TEST_CASE("Verifying the correct behavior of the CEP operator with related values", "[CEP]") {
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr1;
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr2;
	typedef TuplePtr<Tuple<RelatedTuplePtr1, RelatedTuplePtr2>> RelatedTuplePtr;

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >("cep_test.in", "cep_test.res");
	/*
	auto op1_ = std::make_shared<TupleGenerator<OutTuple>>(
			"../../src/test/data/cep_test.in");
	//	qctx.registerOperator("op1", op1_);
	typedef typename RelatedStateValue<InTuple, int, int, 0>::RelatedStateValuePtr Related0;
	typedef typename RelatedStateValue<InTuple, int, int, 0>::RelatedStateValuePtr Related1;
	typedef boost::intrusive_ptr<pfabric::Tuple<Related0, Related1> > RelatedTuple;
	*/
	auto matcher = new Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>(
			Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::FirstMatch);
	auto nfa = matcher->getNFAController();
	auto initState = nfa->createStartState("A");
	auto stateA = nfa->createNormalState("B");
	auto stateB = nfa->createNormalState("C");
	auto stateC = nfa->createFinalState("D");

	auto t4 = nfa->createForwardEdge([&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
		return (getAttribute<0>(*tp) == 1);
	});
	t4->setID(4);
	nfa->createForwardTransition(initState, t4, stateA);

	auto t5 = nfa->createForwardEdge([&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
		return (getAttribute<0>(*tp) == getAttribute<0>(*rt)->getValue() + 1);
	});
	t5->setID(5);
	nfa->createForwardTransition(stateA, t5, stateB);

	auto t6 = nfa->createForwardEdge([&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
		return (getAttribute<0>(*tp) == getAttribute<1>(*rt)->getValue() + 1);
	});
	t6->setID(6);

	nfa->createForwardTransition(stateB, t6, stateC);
	auto init = [&]() {
		RelatedTuplePtr1 related1 = new RelatedStateValuePrevious<InTuplePtr, int, int, 0 >();
		RelatedTuplePtr2 related2 = new RelatedStateValuePrevious<InTuplePtr, int, int, 0 >();
		auto tp = makeTuplePtr(related1, related2);
		return tp;
	};

	auto update = [&](const RelatedTuplePtr& tp, int id, const InTuplePtr& event) {
		if (id == 4) {
			auto rel0 = getAttribute<0>(*tp);
			rel0->updateValue(event);
		}
		else if (id == 5) {
			auto rel1 = getAttribute<1>(*tp);
			rel1->updateValue(event);
		}
	};

	nfa->setDependency(init, update);

	CREATE_DATA_LINK(mockup, matcher);
	CREATE_DATA_LINK(matcher, mockup);

	mockup->start();
}
#endif
