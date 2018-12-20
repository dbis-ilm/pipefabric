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

#include <boost/dll/import.hpp>
#include <boost/variant.hpp>

#include "Plan.hpp"
#include "QueryCompiler.hpp"
#include "SQLParser.hpp"
#include "TopologyBuilder.hpp"
#include "UniqueNameGenerator.hpp"
#include "table/TableInfo.hpp"

namespace dll = boost::dll;

using namespace pfabric;

std::map<std::string, std::string> cppOperatorMap = {
    {"and", "&&"}, {"or", "||"}, {"not", "!"}, {"=", "=="}, {"<>", "!="}};

const std::string cppOperator(const std::string& op) {
  auto it = cppOperatorMap.find(op);
  return (it == cppOperatorMap.end() ? op : it->second);
}

class GenExpressionVisitor : public boost::static_visitor<> {
 public:
  GenExpressionVisitor(std::ostringstream& os) : ostrm(os) {}

  void operator()(sql::Column& col) const {
    ostrm << "get<" << col.columnPosition_ << ">(tp)";
  }

  void operator()(sql::Nil&) const {}
  void operator()(sql::Literal& l) const { ostrm << l; }

  void operator()(sql::Expression& expr) const {
    boost::apply_visitor(*this, expr.head_);
    for (auto& op : expr.tail_) {
      ostrm << " " << cppOperator(op.operator_) << " ";
      boost::apply_visitor(*this, op.operand_);
    }
  }

  void operator()(sql::RightAssocExpression& expr) const {
    boost::apply_visitor(*this, expr.left_);
  }

 private:
  std::ostringstream& ostrm;
};

class MapColumnVisitor : public boost::static_visitor<> {
 public:
  MapColumnVisitor(const std::map<std::string, int>& m)
      : columnMap(std::move(m)) {}

  template <typename T>
  void operator()(T t) const {}

  void operator()(sql::Column& col) const {
    col.columnPosition_ = columnMap.at(col.columnName_);
  }

  void operator()(sql::Expression& expr) const {
    boost::apply_visitor(*this, expr.head_);
    for (auto& op : expr.tail_) {
      boost::apply_visitor(*this, op.operand_);
    }
  }
  void operator()(sql::RightAssocExpression& expr) const {
    boost::apply_visitor(*this, expr.left_);
  }

 private:
  std::map<std::string, int> columnMap;
};

void QueryCompiler::readSettings(const boost::filesystem::path& lib_path) {
  po::options_description desc("Options");
  desc.add_options()("cc", po::value<std::string>(&cc), "c++ compiler")(
      "cflags", po::value<std::string>(&cflags), "c++ flags")(
      "ldflags", po::value<std::string>(&ldflags), "linker flags")(
      "libs", po::value<std::string>(&libs), "linker libraries");
  po::variables_map vm = po::variables_map();

  auto file = lib_path / "config.ini";
  std::ifstream settingsFile(file.string());

  if (settingsFile.fail())
    throw QueryCompileException("cannot find config file");

  po::store(po::parse_config_file(settingsFile, desc), vm);
  po::notify(vm);

  /*
  std::cout << "cc = " << cc
            << "\ncflags = " << cflags
            << "\nldflags = " << ldflags
            << "\nlibs = " << libs << std::endl;
            */
}

void QueryCompiler::traverse(PlanOpPtr op, QueryCompiler::TraverseFunc f) {
  if (!op) return;

  traverse(op->mChild, f);
  traverse(op->mOtherChild, f);

  f(op);
}

void QueryCompiler::checkPlan(PFabricContext& ctx,
                              PlanPtr plan) {
  traverse(plan->sinkOperator(), [&](PlanOpPtr op) {
    switch (op->mOpType) {
      case BasePlanOp::Where_Op: {
        auto whereOp = static_pointer_cast<PlanOp<WhereInfo>>(op);
        // get schema from child
        whereOp->mOutputSchema = whereOp->mChild->mOutputSchema;
        // replace column names in expression by integer positions
        modifyWhereExpression(whereOp);
        break;
      }
      case BasePlanOp::Map_Op: {
        auto mapOp = static_pointer_cast<PlanOp<MapInfo>>(op);
        // construct outputSchema and map columnNames to integer positions
        constructMapSchema(mapOp);
        mTypeMgr.registerType(mapOp->mOutputSchema);
        break;
      }
      case BasePlanOp::FromTable_Op: {
        auto tableOp = static_pointer_cast<PlanOp<FromTableInfo>>(op);
        tableOp->payload().tableInfo =
            ctx.getTableInfo(tableOp->payload().tableName);
        tableOp->mOutputSchema = *(tableOp->payload().tableInfo.get());
        mTableSet.insert(tableOp->payload().tableName);
        mTypeMgr.registerType(tableOp->mOutputSchema);
        break;
      }
      default:
        break;
    }
  });
}

void QueryCompiler::modifyWhereExpression(
    std::shared_ptr<PlanOp<WhereInfo>> whereOp) {
  std::map<std::string, int> columnMap;
  auto inputSchema = whereOp->mChild->mOutputSchema;
  int i = 0;
  for (auto iter = inputSchema.begin(); iter != inputSchema.end();
       iter++, i++) {
    columnMap[iter->getName()] = i;
  }

  MapColumnVisitor visitor(columnMap);
  sql::Expression& expr = whereOp->payload().condition;

  boost::apply_visitor(visitor, expr.head_);
  for (auto& op : expr.tail_) {
    boost::apply_visitor(visitor, op.operand_);
  }
}

void QueryCompiler::constructMapSchema(
    std::shared_ptr<PlanOp<MapInfo>> mapOp) {
  MapInfo& mInfo = mapOp->payload();
  auto inputSchema = mapOp->mChild->mOutputSchema;
  TableInfo::ColumnVector outputColumns;

  for (auto s : mInfo.columns) {
    auto pos = inputSchema.findColumnByName(s);
    if (pos != -1) {
      mInfo.positions.push_back(pos);

      outputColumns.push_back(inputSchema.columnInfo(pos));
    } else {
      std::cout << "column '" << s << "' not found" << std::endl;
      throw QueryCompileException("unknown column");
    }
  }
  //mapOp->mOutputSchema.setColumns(outputColumns);
}

TopologyBuilderPtr QueryCompiler::execQuery(
    PFabricContext& ctx,
    const std::string& queryString) {
  TopologyBuilderPtr topology;

  try {
    const CacheEntry& entry = mCache.findPlanForQuery(queryString);
    topology = entry.builder;
    entry.topology->start(false);

  } catch (std::out_of_range&) {
    auto queryName = compileQuery(ctx, queryString);

    auto queryObj = queryName + "_obj_";
    std::cout << "loadling library: " << queryName << std::endl;
    boost::filesystem::path lib_path(".");
    topology = dll::import<TopologyBuilder>(lib_path / queryName, queryObj,
                                           dll::load_mode::append_decorations);
    std::cout << "creating topology..." << std::endl;
    auto t = topology->create(ctx);
    std::cout << "starting query..." << std::endl;

    CacheEntry entry(topology, t, queryName);
    mCache.addToCache(queryString, entry);

    t->start(false);
  }
  return topology;
}

std::string QueryCompiler::compileQuery(
    PFabricContext& ctx,
    const std::string& queryString) {
  SQLParser parser;
  auto query = parser.parse(queryString);
  auto plan = Plan::constructFromSQLQuery(query);

  checkPlan(ctx, plan);

  std::string cppName = UniqueNameGenerator::instance()->uniqueName("Query");
  generateCode(ctx, plan, cppName);

  auto libName = compileCppCode(".", cppName);
  std::cout << "dll created: " << libName << std::endl;
  return libName;
}

void QueryCompiler::generateCode(
    PFabricContext& ctx, PlanPtr plan,
    const std::string& className) {
  std::string cppFile = className + ".cpp";

  std::ofstream cppStream(cppFile);

  generateHeader(cppStream, className);
  generateTypedefs(cppStream);
  generateBeginClassDefinition(cppStream, className);
  generateQuery(cppStream, ctx, plan);
  generateEndClassDefinition(cppStream, className);
  generateFooter(cppStream, className);
}

void QueryCompiler::generateHeader(std::ostream& os,
                                   const std::string& className) {
  os << "\
#include \"qcomp/TopologyBuilder.hpp\"\n\
\n\
using namespace pfabric;\n\
\n\
BUILDER_CLASS("
     << className << ")\n\n";
}

void QueryCompiler::generateTypedefs(std::ostream& os) {
  for (auto iter = mTypeMgr.begin(); iter != mTypeMgr.end(); iter++) {
    auto tInfo = iter->second;
    os << "typedef " << tInfo.first.generateTypeDef() << " " << tInfo.second
       << ";\n";
  }
  os << "\n";
}

void QueryCompiler::generateBeginClassDefinition(std::ostream& os,
                                                 const std::string& className) {
  os << "PFabricContext::TopologyPtr " << className
     << "::create(PFabricContext& ctx) {\n";
}

void QueryCompiler::generateQuery(std::ostream& os, PFabricContext& ctx,
                                  PlanPtr plan) {
  // generate code for getting table objects
  for (auto tbl : mTableSet) {
    auto tblInfo = ctx.getTableInfo(tbl);

    os << "\tauto " << tbl << " = ctx.getTable<"
       << mTypeMgr.nameOfType(*tblInfo) << "::element_type, " << tblInfo->typeOfKey()
       << ">(\"" << tbl << "\");\n";
  }

  os << "\ttopology = ctx.createTopology();\n";

  traverse(plan->sinkOperator(), [&](PlanOpPtr op) {
    switch (op->mOpType) {
      case BasePlanOp::Where_Op: {
        auto whereOp = static_pointer_cast<PlanOp<WhereInfo>>(op);
        os << "\t\t.where([](auto tp, bool) -> bool {\n\t\t\treturn "
           << generateWhereExpression(whereOp->payload()) << "; })\n";
        break;
      }
      case BasePlanOp::Map_Op: {
        auto mapOp = static_pointer_cast<PlanOp<MapInfo>>(op);
        auto inputSchema = mapOp->mChild->mOutputSchema;
        auto resTypeName = mTypeMgr.nameOfType(mapOp->mOutputSchema);
        os << "\t\t.map<" << resTypeName << ">([](auto tp, bool) -> " << resTypeName << " {\n"
           << "\t\t\treturn makeTuplePtr("
           << generateMapExpression(mapOp->payload()) << "); })\n";
        break;
      }
      case BasePlanOp::FromTable_Op: {
        auto tableOp = static_pointer_cast<PlanOp<FromTableInfo>>(op);
        os << "\ttopology->selectFromTable<"
           << mTypeMgr.nameOfType(tableOp->mOutputSchema) << ", "
           << tableOp->mOutputSchema.typeOfKey() << ">("
           << tableOp->payload().tableName << ")\n";
        break;
      }
      default:
        break;
    }
  });
  os << "\t\t.print();\n";
}

void QueryCompiler::generateEndClassDefinition(std::ostream& os,
                                               const std::string& className) {
  os << "\treturn topology;\n}\n\n";
}

void QueryCompiler::generateFooter(std::ostream& os,
                                   const std::string& className) {
  os << "extern \"C\" BOOST_SYMBOL_EXPORT " << className << " " << className
     << "_obj_;\n"
     << className << " " << className << "_obj_;";
}

std::string QueryCompiler::generateWhereExpression(const WhereInfo& wInfo) {
  std::ostringstream os;

  sql::Expression expr = wInfo.condition;
  GenExpressionVisitor visitor(os);
  boost::apply_visitor(visitor, expr.head_);
  for (auto& op : expr.tail_) {
    os << " " << cppOperator(op.operator_) << " ";
    boost::apply_visitor(visitor, op.operand_);
  }

  return os.str();
}

std::string QueryCompiler::generateMapExpression(const MapInfo& mInfo) {
  std::ostringstream os;
  bool first = true;

  for (auto p : mInfo.positions) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "get<" << p << ">(tp)";
  }
  return os.str();
}

std::string QueryCompiler::compileCppCode(
    const boost::filesystem::path& lib_path,
    const std::string& cppFileName) {
  std::ostringstream cmd;
  auto pos = cppFileName.find(".cpp");
  std::string fileName = cppFileName.substr(0, pos);

  cmd << cc << " " << cflags << " -Wno-#pragma-messages "
      << "-o " << lib_path.string() << "/lib" << fileName
      << ".dylib -install_name @rpath/lib" << fileName << ".dylib "
      << lib_path / fileName << ".cpp " << ldflags << " " << libs << std::endl;
  // std::cout << cmd.str() << std::endl;
  std::system(cmd.str().c_str());
  return fileName;
}
