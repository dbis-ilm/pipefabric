/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

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
