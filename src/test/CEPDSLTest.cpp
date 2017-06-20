#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include "cep/Matcher.hpp"
#include "cep/NFAController.hpp"
#include "cep/dsl/CEPState.hpp"

using namespace pfabric;

typedef TuplePtr<int, int, int> InTuplePtr;
typedef TuplePtr<int, int, int> OutTuplePtr;
typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;

TEST_CASE("Defining states and transitions using a DSL", "[CEP]") {
  typedef CEPState<InTuplePtr, RelatedTuplePtr> MyCEPState;
  MyCEPState start;
  MyCEPState a([](auto tp, auto rt) { return (get<0>(*tp) == 1); });
  MyCEPState b([](auto tp, auto rt) { return (get<0>(*tp) == 2); });
  MyCEPState c([](auto tp, auto rt) { return (get<0>(*tp) == 3); });
  MyCEPState d([](auto tp, auto rt) { return (get<0>(*tp) == 4); });
  MyCEPState end([](auto tp, auto rt) { return (get<0>(*tp) == 5); }, MyCEPState::Stopp);

  auto expr = start >> a >> (b || c) >> !d >> end;
  std::cout << "-------------------------" << std::endl;
  expr.print();

  auto matcher = std::make_shared<Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>>(
			Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::FirstMatch);
  matcher->constructNFA(expr);
}
