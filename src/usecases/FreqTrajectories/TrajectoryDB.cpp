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

#include "TrajectoryDB.hpp"

void createTables(PFabricContext& ctx, const std::string& landmarkFile) {
  {
    TableInfo tblInfo("visits",
      { ColumnInfo("pointID", ColumnInfo::UInt_Type),
        ColumnInfo("count", ColumnInfo::UInt_Type) },
        ColumnInfo::UInt_Type);

    auto visitsTable = ctx.createTable<Visit::element_type, uint_t>(tblInfo);
  }

  auto tracksTable = ctx.createTable<UserTrack::element_type, uint_t>("user_tracks");

  {
    TableInfo tblInfo("landmarks",
      { ColumnInfo("pointID", ColumnInfo::UInt_Type),
        ColumnInfo("latitude", ColumnInfo::Double_Type),
        ColumnInfo("longitude", ColumnInfo::Double_Type),
        ColumnInfo("description", ColumnInfo::String_Type) },
        ColumnInfo::UInt_Type);

    auto landmarksTable = ctx.createTable<Landmark::element_type, uint_t>(tblInfo);
    std::cout << "# of landmarks: " << landmarksTable->size() << std::endl;
    if (landmarksTable->size() == 0 && landmarkFile != "") {
      // load data from csv file
      auto t1 = ctx.createTopology();
      auto s = t1->newStreamFromFile(landmarkFile)
        .extract<Landmark>(',')
        .keyBy<uint_t>([](auto tp) { return get<0>(tp); })
        .toTable<uint_t>(landmarksTable);
        t1->start(false);

        // just to check whether we have loaded the data
        auto t2 = ctx.createTopology();
        auto q = t2->selectFromTable<Landmark, uint_t>(landmarksTable)
        .print();
        t2->start(false);
    }
  }
}
