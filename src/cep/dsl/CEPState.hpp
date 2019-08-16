/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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

#ifndef CEPState_hpp_
#define CEPState_hpp_

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <exception>

#include "fmt/format.h"

namespace pfabric {

  template <typename Tin, typename Tstate>
  class CEPState {
  public:
      typedef CEPState<Tin, Tstate>* CEPStatePtr;

      enum Op { SEQ, OR, NOT, START, END };
      enum StateTag { Start, Stopp, Intermediate };

      struct Expr {
        Op op;
        CEPStatePtr fromState, toState;
        Expr(Op o, CEPStatePtr fptr, CEPStatePtr tptr = nullptr) :
        op(o), fromState(fptr), toState(tptr) {}
      };
      typedef std::vector<Expr> ExprList;

      ExprList& exprTable() { return exprs; }

      typedef std::function<bool(const Tin& tp, const Tstate& related)> Predicate;

      CEPState(Predicate p, StateTag t = Intermediate) : pred(p), tag(t) {
        id = (t == Stopp ? 1000 : ++globalId);
        if (t == Stopp)
          exprs.push_back(Expr(END, this));
      }

      CEPState() : pred(nullptr), id(0), tag(Start) {
        exprs.push_back(Expr(START, this));
      }

      CEPState& operator>> (CEPState& other) {
        other.exprs.insert(other.exprs.end(), exprs.begin(), exprs.end());
        other.exprs.push_back(Expr(SEQ, this, &other));
        return other;
      }
      CEPState& operator|| (CEPState& other) {
        exprs.insert(exprs.end(), other.exprs.begin(), other.exprs.end());
        exprs.push_back(Expr(OR, this, &other));
        return *this;
      }
      CEPState& operator! () {
        exprs.push_back(Expr(NOT, this));
        return *this;
      }

      void print() {
        std::cout << "STATE: " << id << std::endl;
        for (auto& e : exprs) {
          std::cout << e.op << "(" << e.fromState->id;
          if (e.toState)
            std::cout << "," << e.toState->id;
          std::cout << ")" << std::endl;
        }
      }
      Predicate predicate() const { return pred; }
      int ID() const { return id; }

  protected:
    Predicate pred;
    int id;
    StateTag tag;
    ExprList exprs;

    static int globalId;
  };

  template <typename Tin, typename Tstate>
  int CEPState<Tin,Tstate>::globalId = 0;

}

#endif
