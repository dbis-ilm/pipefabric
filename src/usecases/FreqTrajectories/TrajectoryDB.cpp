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
