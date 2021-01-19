/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

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
