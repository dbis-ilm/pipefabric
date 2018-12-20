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

#ifndef SQLParser_hpp_
#define SQLParser_hpp_

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include "QueryCompileException.hpp"

namespace pfabric {

  namespace sql {

  struct SelectClause {
    std::vector<std::string> columnList_;
  };

  struct FromClause {
    std::string tableName_;
  };

  struct Nil;
  struct Literal;
  struct Column;
  struct RightAssocExpression;
  struct Expression;

  typedef boost::variant<
           Nil
         , Literal
         , Column
         , boost::recursive_wrapper< RightAssocExpression >
         , boost::recursive_wrapper< Expression >
       >
   Operand;

  struct Nil {};

  struct Literal {
      int value_;
  };

  struct Column {
      std::string columnName_;
      int columnPosition_;
  };

  struct Operation {
      std::string operator_;
      Operand operand_;
  };

  struct RightAssocExpression {
      Operand left_;
    //  boost::optional< Operand > right_;
  };

  struct Expression {
    Operand head_;
    std::list<Operation> tail_;
  };

  struct WhereClause {
      Expression condition_;
  };

  struct SQLQuery {
    SelectClause selectClause_;
    FromClause fromClause_;
    boost::optional<WhereClause> whereClause_;
  };

  typedef std::shared_ptr<SQLQuery> SQLQueryPtr;

  inline std::ostream& operator<<(std::ostream& out, Nil) {
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const Literal& lit) {
    out << lit.value_;
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const Column& col) {
    out << col.columnName_;
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const Operation& op) {
    out << op.operator_ << " " << op.operand_;
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const RightAssocExpression& expr) {
    out << expr.left_;
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const Expression& expr) {
    out << expr.head_;
    for (auto& op : expr.tail_) {
      out << " " << op;
    }
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const SelectClause& c) {
    out << "SELECT ";
    for (std::size_t i = 0; i < c.columnList_.size(); i++) {
      if (i > 0) out << ", ";
      out << c.columnList_[i];
    }
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const FromClause& c) {
    out << "FROM " << c.tableName_;
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, const WhereClause& c) {
    out << "WHERE " << c.condition_;
    return out;
  }

  inline std::ostream& operator<<(std::ostream& out, SQLQuery& q) {
    out << q.selectClause_ << " " << q.fromClause_ << " " << q.whereClause_;
    return out;
  }
}

class SQLParser {
public:
  SQLParser() {}

  sql::SQLQueryPtr parse(const std::string& stmt);
};

}

#endif
