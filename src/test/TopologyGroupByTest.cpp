#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include <boost/filesystem.hpp>

#include "core/Tuple.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"

using namespace pfabric;
using namespace ns_types;

template<typename StreamElement>
class MyAggregateState: public AggregateStateBase<StreamElement> {
public:
	int group1_;
	AggrSum<double> sum1_;

	MyAggregateState() { init(); }

	virtual void init() override {
		group1_ = 0;
		sum1_.init();
	}
};

TEST_CASE("Building and running a topology with simple unpartitioned grouping", 
        "[Simple unpartitioned Grouping]") {
  typedef TuplePtr<Tuple<unsigned long, double>> MyTuplePtr;
  typedef TuplePtr<Tuple<double> > AggregationResultPtr;
  typedef Aggregator1<MyTuplePtr, AggrSum<double>, 1> AggrStateSum;
        
  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    if (n<500) return makeTuplePtr((unsigned long)0, (double)n + 0.5);
    else return makeTuplePtr((unsigned long)n, (double)n + 0.5);
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<double> results;

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .groupBy<AggregationResultPtr, AggrStateSum, unsigned long>()
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num)
          results.push_back(get<0>(tp));
        tuplesProcessed++;
    });
	//.print(std::cout);

  t.start(false);

  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == num);

  for (auto i=0u; i<num; i++) {
    if (i==0) REQUIRE(results[i] == 0.5);
    else if (i<500) REQUIRE(results[i] == results[i-1]+i+0.5);
	else if (i==500) REQUIRE(results[i] == 500.5);
	else REQUIRE(results[i] == results[i-1]+1.0);
  }
}

TEST_CASE("Building and running a topology with unpartitioned grouping", 
        "[Unpartitioned Grouping]") {
  typedef TuplePtr<Tuple<unsigned long, double>> MyTuplePtr;
  typedef MyAggregateState<const MyTuplePtr&> MyAggrState;
  typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	
  auto finalFun = [&](MyAggrStatePtr myState) {
                      auto tp = makeTuplePtr(myState->group1_, myState->sum1_.value());
                      return tp;
                  };
											 
  auto iterFun = [&](const MyTuplePtr& tp, MyAggrStatePtr myState, const bool outdated) {
			          myState->group1_ = get<0>(tp);
			          myState->sum1_.iterate(get<1>(tp), outdated);
		          };
	
  typedef TuplePtr<Tuple<int, double>> AggregationResultPtr;
        
  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    if (n<500) return makeTuplePtr((unsigned long)0, (double)n + 0.5);
    else return makeTuplePtr((unsigned long)n, (double)n + 0.5);
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<std::vector<double>> results;

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .groupBy<AggregationResultPtr, MyAggrState, unsigned long>(finalFun, iterFun)
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num) {
		  std::vector<double> tmpVec;
		  tmpVec.push_back(get<0>(tp));
		  tmpVec.push_back(get<1>(tp));

          results.push_back(tmpVec);
		}
        tuplesProcessed++;
    });
	//.print(std::cout);

  t.start(false);

  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == num);

  for (auto i=0u; i<num; i++) {
    if (i==0) { 
	  REQUIRE(results[i][0] == 0);
	  REQUIRE(results[i][1] == 0.5); 
	} else if (i<500) {
	  REQUIRE(results[i][0] == 0);
	  REQUIRE(results[i][1] == results[i-1][1]+i+0.5); 
	} else if (i==500) {
	  REQUIRE(results[i][0] == i); 
	  REQUIRE(results[i][1] == 500.5); 
	} else { 
	  REQUIRE(results[i][0] == i);
	  REQUIRE(results[i][1] == results[i-1][1]+1.0); 
	}
  }
}

TEST_CASE("Building and running a topology with partitioned grouping", 
        "[Partitioned Grouping]") {
  typedef TuplePtr<Tuple<unsigned long, double>> MyTuplePtr;
  typedef MyAggregateState<const MyTuplePtr&> MyAggrState;
  typedef std::shared_ptr<MyAggrState> MyAggrStatePtr;
	
  auto finalFun = [&](MyAggrStatePtr myState) {
                      auto tp = makeTuplePtr(myState->group1_, myState->sum1_.value());
                      return tp;
                  };
											 
  auto iterFun = [&](const MyTuplePtr& tp, MyAggrStatePtr myState, const bool outdated) {
			          myState->group1_ = get<0>(tp);
			          myState->sum1_.iterate(get<1>(tp), outdated);
		          };
	
  typedef TuplePtr<Tuple<int, double>> AggregationResultPtr;
        
  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    if (n<500) return makeTuplePtr((unsigned long)0, (double)n + 0.5);
    else return makeTuplePtr((unsigned long)n, (double)n + 0.5);
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<std::vector<double>> results;

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
	.partitionBy([](auto tp) { return get<0>(tp) % 5; }, 5)
    .groupBy<AggregationResultPtr, MyAggrState, unsigned long>(finalFun, iterFun)
    .merge()
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num) {
		  std::vector<double> tmpVec;
		  tmpVec.push_back(get<0>(tp));
		  tmpVec.push_back(get<1>(tp));

          results.push_back(tmpVec);
		}
        tuplesProcessed++;
    });
	//.print(std::cout);

  t.start(false);

  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == num);
	
//sorting (because of unordered results after partitioning)
  std::vector<std::vector<double>> resultsFirst;
  std::vector<std::vector<double>> resultsSecond;
  for(auto i=0u; i<results.size(); i++) {
	  if(results[i][0]==0) resultsFirst.push_back(results[i]);
	  else resultsSecond.push_back(results[i]);
  }

  std::sort(resultsFirst.begin(), resultsFirst.end(), [](const std::vector<double>& a,
											   const std::vector<double>& b) {return a[1] < b[1]; });
  std::sort(resultsSecond.begin(), resultsSecond.end(), [](const std::vector<double>& a,
											   const std::vector<double>& b) {return a[1] < b[1]; });

  std::vector<std::vector<double>> finalResults;
  for(auto i=0u; i<resultsFirst.size(); i++) {
	  finalResults.push_back(resultsFirst[i]);
  }
  for(auto i=0u; i<resultsSecond.size(); i++) {
	  finalResults.push_back(resultsSecond[i]);
  }
//end of sorting

  //requirement checks
  for (auto i=0u; i<num; i++) {
    if (i==0) {
	  REQUIRE(finalResults[i][0] == 0);
	  REQUIRE(finalResults[i][1] == 0.5);
	} else if (i<500) {
	  REQUIRE(finalResults[i][0] == 0);
	  REQUIRE(finalResults[i][1] == finalResults[i-1][1]+i+0.5);
	} else if (i==500) {
	  REQUIRE(finalResults[i][0] == i);
	  REQUIRE(finalResults[i][1] == 500.5);
	} else {
	  REQUIRE(finalResults[i][0] == i);
	  REQUIRE(finalResults[i][1] == finalResults[i-1][1]+1.0);
	}
  }
}