#ifndef CEPExpr_hpp_
#define CEPExpr_hpp_

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <exception>

#include "format/format.hpp"

namespace pfabric {

  class InvalidCEPException : public std::exception {
    std::string msg;

  public:
      InvalidCEPException(const char *s = "") : msg(s) {}

      virtual const char* what() const throw() {
        return fmt::format("InvalidCEPException: {}", msg).c_str();
      }
  };

struct CEPExpr {
  enum ExprTag { Unknown, State, Seq, Or, And };

  virtual ExprTag tag() const { return Unknown; }
};

typedef std::shared_ptr<CEPExpr> CEPExprPtr;

struct CEPState : public CEPExpr {
  std::string id;

  CEPState(const char *s) : id(s) { std::cout << "State(" << s << "): " << tag() << std::endl; }

  virtual ExprTag tag() const { return State; }
};

struct SEQExpr : public CEPExpr {
  std::vector<CEPExprPtr> sequence;

  SEQExpr(std::initializer_list<CEPExprPtr> s) : sequence(s) {}

  virtual ExprTag tag() const { return Seq; }

};

struct ORExpr : public CEPExpr {
  std::vector<CEPExprPtr> sequence;

  ORExpr(std::initializer_list<CEPExprPtr> s) : sequence(s) {}

  virtual ExprTag tag() const { return Or; }
};

inline CEPExprPtr _S(const char *s) { return CEPExprPtr(new CEPState(s)); }
inline CEPExprPtr SEQ(std::initializer_list<CEPExprPtr> l) { return CEPExprPtr(new SEQExpr(l));  }
inline CEPExprPtr OR(std::initializer_list<CEPExprPtr> l) { return CEPExprPtr(new ORExpr(l)); }

}

#endif
