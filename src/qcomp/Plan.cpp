#include "Plan.hpp"

using namespace pfabric;

void BasePlanOp::addChild(PlanOpPtr c) {
  mChild = c;
  c->mParent = shared_from_this();
}

PlanPtr Plan::constructFromSQLQuery(const sql::SQLQueryPtr& q) {
  auto tbl = std::make_shared<PlanOp<FromTableInfo>>(BasePlanOp::FromTable_Op);
  FromTableInfo info1;
  info1.tableName = q->fromClause_.tableName_;
  tbl->payload(info1);

  PlanOpPtr next = tbl;

  if (q->whereClause_) {
    auto sel = std::make_shared<PlanOp<WhereInfo>>(BasePlanOp::Where_Op);
    WhereInfo info2;
    info2.condition = q->whereClause_->condition_;
    sel->payload(info2);
    sel->addChild(tbl);
    next = sel;
  }

  if (q->selectClause_.columnList_.size() == 1 && q->selectClause_.columnList_[0] == "*") {
    // we don't need a map operator here - do nothing
  }
  else {
    auto proj = std::make_shared<PlanOp<MapInfo>>(BasePlanOp::Map_Op);
    MapInfo info3;
    info3.columns = q->selectClause_.columnList_;
    proj->payload(info3);
    proj->addChild(next);
    next = proj;
  }

  auto plan = std::make_shared<Plan>();
  plan->mSink = next;
  return plan;
}
