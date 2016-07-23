#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include "core/Tuple.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/SHJoin.hpp"
#include "qop/SlidingWindow.hpp"


using namespace pfabric;


typedef Tuple<int, int, int, int> ResTuple;
typedef TuplePtr<ResTuple> ResTuplePtr;

typedef Tuple<int, int> MyTuple;
typedef TuplePtr<MyTuple> MyTuplePtr;


template<
	typename ResultType
>
class TupleGenerator :
	public DataSource< MyTuplePtr >,
	public DataSink< ResultType >
{
private:
	typedef DataSource<MyTuplePtr> SourceBase;
	typedef DataSink< ResultType > SinkBase;

	typedef typename SinkBase::InputDataChannel InputDataChannel;
	typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel;
	typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

public:

	TupleGenerator() : mTuplesProcessed(0), mOutdatedTuplesProcessed(0) {}


	BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, TupleGenerator, processDataElement);
	BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, TupleGenerator, processPunctuation);

	void start(int num, bool reset = true) {
		if (reset) {
			mOutdatedTuplesProcessed = mTuplesProcessed = 0;
		}
		for (int i = 0; i < num; i++) {
			auto tp = makeTuplePtr(i, i);
			getOutputDataChannel().publish(tp, false);
		}
	}

	int  numProcessedTuples() const { return mTuplesProcessed; }
	int  numOutdatedTuples() const { return mOutdatedTuplesProcessed; }

private:

	void processPunctuation( const PunctuationPtr& punctuation ) {}

	void processDataElement( const ResultType& data, const bool outdated ) {
		REQUIRE( (getAttribute<0>( data ) == getAttribute<2>( data )) );
		if (outdated) {
			mOutdatedTuplesProcessed++;
		}
		else {
			mTuplesProcessed++;
		}
	}

	int mTuplesProcessed, mOutdatedTuplesProcessed;
};


/**
 * A simple test of the symmetric hash join.
 */
TEST_CASE("Joining two streams using sliding windows", "[SHJoin]") {
	typedef SlidingWindow<MyTuplePtr> TestWindow;
	typedef SHJoin< MyTuplePtr, MyTuplePtr > TestJoin;
	typedef TupleGenerator< typename TestJoin::ResultElement > TestGenerator;

	auto tgen = std::make_shared<TestGenerator>();
	auto win1 = std::make_shared< TestWindow >(WindowParams::RowWindow, 10);
	auto win2 = std::make_shared< TestWindow >(WindowParams::RowWindow, 10);
	auto hfun = [&](const MyTuplePtr& tp) { return (unsigned long) getAttribute<0>(tp); };
	auto join_pred = [&](const MyTuplePtr& tp1, const MyTuplePtr& tp2) {
		return getAttribute<0>(tp1) == getAttribute<0>(tp2);
	};
	auto join = std::make_shared< TestJoin >(hfun, hfun, join_pred);

	CREATE_DATA_LINK(tgen, win1);
	CREATE_DATA_LINK(tgen, win2);
	connectChannels(win1->getOutputDataChannel(), join->getLeftInputDataChannel());
	connectChannels(win2->getOutputDataChannel(), join->getRightInputDataChannel());

	CREATE_DATA_LINK(join, tgen);

	// we send 10 tuples to the join
	tgen->start(10);
	REQUIRE(tgen->numProcessedTuples() == 10);
}

/**
 * Another simple test of the symmetric hash join.
 */
TEST_CASE("Joining two streams with outdated items using sliding windows", "[SHJoin]") {
	typedef SlidingWindow<MyTuplePtr> TestWindow;
	typedef SHJoin< MyTuplePtr, MyTuplePtr > TestJoin;
	typedef TupleGenerator< typename TestJoin::ResultElement > TestGenerator;

	auto tgen1 = std::make_shared<TestGenerator>();
	auto tgen2 = std::make_shared<TestGenerator>();
	auto win1 = std::make_shared<TestWindow>(WindowParams::RowWindow, 10);
	auto win2 = std::make_shared<TestWindow>(WindowParams::RowWindow, 10);

	auto hfun = [&]( const MyTuplePtr& tp ) { return (unsigned long) getAttribute<0>(tp); };
	auto join_pred = [&](const MyTuplePtr& tp1, const MyTuplePtr& tp2) { return true; };
	auto join = std::make_shared< TestJoin >(hfun, hfun, join_pred);

	CREATE_DATA_LINK(tgen1, win1);
	CREATE_DATA_LINK(tgen2, win2);
	connectChannels(win1->getOutputDataChannel(), join->getLeftInputDataChannel());
	connectChannels(win2->getOutputDataChannel(), join->getRightInputDataChannel());
	CREATE_DATA_LINK(join, tgen1);

	// we send 5 tuples for stream #1
	tgen1->start(5);

	// and 10 corresponding tuples for stream #2
	tgen2->start(10);

	REQUIRE(tgen1->numProcessedTuples() == 5);

	// and again 5 tuples for stream #1
	tgen1->start(5, false);

	REQUIRE(tgen1->numProcessedTuples() == 10);
}
