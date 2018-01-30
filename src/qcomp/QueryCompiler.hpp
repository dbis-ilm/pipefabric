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

#ifndef QueryCompiler_hpp_
#define QueryCompiler_hpp_

#include <string>
#include <set>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "dsl/PFabricContext.hpp"
#include "Plan.hpp"
#include "QueryCompileException.hpp"
#include "TypeManager.hpp"
#include "TopologyBuilder.hpp"
#include "PlanCache.hpp"

namespace po = boost::program_options;

namespace pfabric {

 class QueryCompiler {
    typedef std::function<void(PlanOpPtr)> TraverseFunc;

  public:
    QueryCompiler() {}

    void readSettings(const boost::filesystem::path& lib_path) throw (QueryCompileException);
 
    std::string compileQuery(PFabricContext& ctx, const std::string& queryString) throw (QueryCompileException);
    TopologyBuilderPtr execQuery(PFabricContext& ctx, const std::string& queryString) throw (QueryCompileException);

  private:
    void checkPlan(PFabricContext& ctx, PlanPtr plan) throw(QueryCompileException);
    void traverse(PlanOpPtr op, TraverseFunc f);

    void modifyWhereExpression(std::shared_ptr<PlanOp<WhereInfo>> whereOp);
    void constructMapSchema(std::shared_ptr<PlanOp<MapInfo>> mapOp) throw (QueryCompileException);

    void generateCode(PFabricContext& ctx, PlanPtr plan, const std::string& cppFile) throw (QueryCompileException);
    void generateHeader(std::ostream& os, const std::string& className); 
    void generateFooter(std::ostream& os, const std::string& className); 
    void generateTypedefs(std::ostream& os);
    void generateQuery(std::ostream& os, PFabricContext& ctx, PlanPtr plan);
    void generateBeginClassDefinition(std::ostream& os, const std::string& className);
    void generateEndClassDefinition(std::ostream& os, const std::string& className);

    std::string generateWhereExpression(const WhereInfo& wInfo);
    std::string generateMapExpression(const MapInfo& mInfo);

    std::string compileCppCode(const boost::filesystem::path& lib_path, const std::string& cppFileName) throw (QueryCompileException);

    std::string cc, cflags, ldflags, libs;
    std::set<std::string> mTableSet;
    TypeManager mTypeMgr;
    PlanCache mCache;
  };
}

#endif
