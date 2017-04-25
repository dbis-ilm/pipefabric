#ifndef Plan_hpp_
#define Plan_hpp_

#include <memory>
#include <boost/variant.hpp>
#include "table/TableInfo.hpp"

#include "SQLParser.hpp"

namespace pfabric {

  struct FromTableInfo {
    std::string tableName;
    TableInfoPtr tableInfo;
  };

  struct WhereInfo {
    sql::Expression condition;
  };

  struct MapInfo {
    std::vector<std::string> columns;
    std::vector<int> positions;
  };

  /* ------------------------------------------------------------------------ */

  class BasePlanOp;
  typedef std::shared_ptr<BasePlanOp> PlanOpPtr;

  class Plan;
  typedef std::shared_ptr<Plan> PlanPtr;

  class BasePlanOp : public std::enable_shared_from_this<BasePlanOp>  {
  friend class Plan;
  friend class QueryCompiler;

  public:
    enum OpType { FromTable_Op, Where_Op, Map_Op };

    void addChild(PlanOpPtr c);
    
  protected:
    BasePlanOp(OpType o) : mOpType(o) {}

    OpType mOpType;
    TableInfo mOutputSchema;
    std::weak_ptr<BasePlanOp> mParent;
    PlanOpPtr mChild, mOtherChild;
   };

  template <typename Payload>
  class PlanOp : public BasePlanOp {
  public:
    PlanOp(BasePlanOp::OpType o) : BasePlanOp(o) {}

    const Payload& payload() const { return mPayload; }
    Payload& payload() { return mPayload; }

    void payload(const Payload& p) { mPayload = p; }

  private:
    Payload mPayload;
  };

  /* ------------------------------------------------------------------------ */

  class Plan {
  public:
    static PlanPtr constructFromSQLQuery(const sql::SQLQueryPtr& q);

    PlanOpPtr sinkOperator() { return mSink; }

  private:
    PlanOpPtr mSink;
  };

}

#endif
