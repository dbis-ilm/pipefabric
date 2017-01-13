#ifndef QueryCompiler_hpp_
#define QueryCompiler_hpp_

#include <string>
#include <set>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "topology/PFabricContext.hpp"
#include "Plan.hpp"
#include "QueryCompileException.hpp"
#include "TypeManager.hpp"
#include "TopologyBuilder.hpp"

namespace po = boost::program_options;

namespace pfabric {

  typedef boost::shared_ptr<TopologyBuilder> TopologyBuilderPtr;

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
  };
}

#endif
