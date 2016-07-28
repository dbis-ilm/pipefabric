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
typedef Tuple<int, int, int, std::string, unsigned long> OutTuple;
typedef TuplePtr<OutTuple> OutTuplePtr;

/*
typedef Tuple<int> RelatedTuple;
typedef TuplePtr<RelatedTuple> RelatedTuplePtr;
*/

TEST_CASE("Verifying the correct behavior of the CEP operator", "[CEP]") {

	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;

	// input: cep_test.in; output: cep_test.res
	std::ifstream input("cep_test.in");
	REQUIRE(input.is_open());

	std::ifstream expected("cep_test.res");
	REQUIRE(expected.is_open());

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >(input, expected);

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

#if 0
BOOST_AUTO_TEST_CASE(RelatedSeqCEPTest) {

	auto op1_ = std::make_shared<TupleGenerator<OutTuple>>(
			"../../src/test/data/cep_test.in");
	//	qctx.registerOperator("op1", op1_);
	typedef typename RelatedStateValue<InTuple, int, int, 0>::RelatedStateValuePtr Related0;
	typedef typename RelatedStateValue<InTuple, int, int, 0>::RelatedStateValuePtr Related1;
	typedef boost::intrusive_ptr<pfabric::Tuple<Related0, Related1> > RelatedTuple;
	auto op2_ = new Matcher<InTuple, OutTuple, RelatedTuple>(
			Matcher<InTuple, OutTuple, RelatedTuple>::FirstMatch);
	makeLink(op1_, op2_);
	NFAController<InTuple, OutTuple, RelatedTuple>::NFAControllerPtr nfa =
			op2_->getNFAController();
	StartState<InTuple, OutTuple, RelatedTuple>::StartStatePtr stateinit_ =
			nfa->createStartState("A");
	NormalState<InTuple, OutTuple, RelatedTuple>::NormalStatePtr stateA =
			nfa->createNormalState("B");
	NormalState<InTuple, OutTuple, RelatedTuple>::NormalStatePtr stateB =
			nfa->createNormalState("C");
	FinalState<InTuple, OutTuple, RelatedTuple>::FinalStatePtr stateC =
			nfa->createFinalState("D");
	ForwardEdge<InTuple, OutTuple, RelatedTuple>::ForwardEdgePtr t4 =
			nfa->createForwardEdge(
					[&](const InTuplePtr& tp, const RelatedTuple& rt) {return (std::get<0>(*tp) == 1);});
	t4->setID(4);
	nfa->createForwardTransition(stateinit_, t4, stateA);
	ForwardEdge<InTuple, OutTuple, RelatedTuple>::ForwardEdgePtr t5 =
			nfa->createForwardEdge(
					[&](const InTuplePtr& tp, const RelatedTuple& rt) {return (std::get<0>(*tp) == std::get<0>(*rt)->getValue() + 1);});
	t5->setID(5);
	nfa->createForwardTransition(stateA, t5, stateB);
	ForwardEdge<InTuple, OutTuple, RelatedTuple>::ForwardEdgePtr t6 =
			nfa->createForwardEdge(
					[&](const InTuplePtr& tp, const RelatedTuple& rt) {return (std::get<0>(*tp) == std::get<1>(*rt)->getValue() + 1);});
	t6->setID(6);
	nfa->createForwardTransition(stateB, t6, stateC);
	auto init =
			[&]() {

				Related0 related0 = Related0 (new RelatedStateValuePrevious<InTuple, int, int, 0 >() );
				Related1 related1 = Related1 (new RelatedStateValuePrevious<InTuple, int, int, 0 >() );
				RelatedTuple tp = makeTuplePtr(related0,related1);
				return tp;
			};
	auto update =
			[&](const RelatedTuple& tp, int id, const InTuplePtr& event ) {
				if(id == 4) {
					auto rel0 = std::get< 0 >(*tp);
					rel0->updateValue(event);
				}

				else if(id == 5) {
					auto rel1 = std::get< 1 >(*tp);
					rel1->updateValue(event);
				}

			};

	nfa->setDependency(init, update);

	connectChannels(op2_->getOutputChannel<0>(), op1_->getInputChannel<0>());
	op1_->start();
	op1_->checkListResult("../../src/test/data/cep_test.res");
}
#endif
