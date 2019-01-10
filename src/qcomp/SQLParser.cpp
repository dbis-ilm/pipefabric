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

#include <iostream>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/any.hpp>
#include <boost/foreach.hpp>

#include "SQLParser.hpp"

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::SelectClause,
    (std::vector<std::string>, columnList_)
)

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::FromClause,
    (std::string, tableName_)
)

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::Literal,
    ( int, value_ )
)

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::Column,
    ( std::string, columnName_ )
)

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::Operation,
    ( std::string, operator_ )
    ( pfabric::sql::Operand, operand_ )
)

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::RightAssocExpression,
    (pfabric::sql::Operand, left_)//,
//    (boost::optional< sql::Operand >, right_)
)
BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::Expression,
    (pfabric::sql::Operand, head_),
    (std::list<pfabric::sql::Operation>, tail_)
)

BOOST_FUSION_ADAPT_STRUCT(
    pfabric::sql::WhereClause,
    (pfabric::sql::Expression, condition_)
)

BOOST_FUSION_ADAPT_STRUCT(
  pfabric::sql::SQLQuery,
  (pfabric::sql::SelectClause, selectClause_),
  (pfabric::sql::FromClause, fromClause_),
  (boost::optional<pfabric::sql::WhereClause>, whereClause_)
)

  namespace pfabric {
namespace sql {
  template <typename Iterator>
  struct sql_grammar : qi::grammar<Iterator, SQLQuery(), ascii::space_type> {
         sql_grammar() : sql_grammar::base_type(start) {
           qi::lit_type lit_;
           qi::string_type string_;
           qi::int_type int_;

             using qi::lit;
             using qi::alpha;
             using qi::char_;
             using qi::alnum;
             using qi::lexeme;
             using phoenix::at_c;
             using namespace qi::labels;

            identifier %= lexeme[ (alpha >> *(alnum | qi::char_('_'))) ];

            // SELECT clause
            column_list = identifier >> *(',' >> identifier);

            star = "*";

            select_clause = lit("select") >> (column_list | star);

            // FROM clause
            table_name %= identifier;

            from_clause = lit("from") >> table_name;

            // WHERE clause
            expression = relational_expression
                >> *( (string_("and") > relational_expression )
                    | (string_( "or" ) > relational_expression )
                );

            relational_expression = additive_expression
            >> *(   (string_( "<=" ) > additive_expression )
                |   (string_( "<" ) > additive_expression )
                |   (string_( ">=" ) > additive_expression )
                |   (string_( ">" ) > additive_expression )
                |   (string_( "<>" ) > additive_expression )
                |   (string_( "=" ) > additive_expression )
                );

            additive_expression = primary_expression
              ;

            primary_expression = column_name
              | literal
              ;

            literal = int_
              ;

            column_name = identifier;

            where_clause = lit("where") >> expression;

            start %= select_clause >> from_clause >> -where_clause;

            BOOST_SPIRIT_DEBUG_NODE( expression );
            BOOST_SPIRIT_DEBUG_NODE( relational_expression );
            BOOST_SPIRIT_DEBUG_NODE( additive_expression );
            BOOST_SPIRIT_DEBUG_NODE( primary_expression );

         }

         qi::rule<Iterator, Expression(), ascii::space_type> expression;
         qi::rule<Iterator, Expression(), ascii::space_type> relational_expression;
         qi::rule<Iterator, RightAssocExpression(), ascii::space_type> additive_expression;
         qi::rule<Iterator, Literal(), ascii::space_type> literal;
         qi::rule<Iterator, Column(), ascii::space_type> column_name;
         qi::rule<Iterator, Operand(), ascii::space_type> primary_expression;
         qi::rule<Iterator, std::string(), ascii::space_type> star;
         qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> column_list;
         qi::rule<Iterator, std::string(), ascii::space_type> identifier;
         qi::rule<Iterator, std::string(), ascii::space_type> table_name;
         qi::rule<Iterator, SelectClause(), ascii::space_type> select_clause;
         qi::rule<Iterator, FromClause(), ascii::space_type> from_clause;
         qi::rule<Iterator, WhereClause(), ascii::space_type> where_clause;
         qi::rule<Iterator, SQLQuery(), ascii::space_type> start;
     };
}
}

using namespace pfabric;

sql::SQLQueryPtr SQLParser::parse(const std::string& stmt) {
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef sql::sql_grammar<iterator_type> sql_parser;

  std::string::const_iterator iter = stmt.begin();
  std::string::const_iterator end = stmt.end();
  sql::SQLQueryPtr res = std::make_shared<sql::SQLQuery>();
  sql_parser parser;

  bool r = phrase_parse(iter, end, parser, space, *res);
   if (r && iter == end) {
     // success
   }
   else {
     std::cout << "parsing failed: " << std::string(iter, end) << std::endl;
     throw QueryCompileException("parsing failed");
   }
   return res;
}
