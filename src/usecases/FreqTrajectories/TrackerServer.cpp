#include <limits>
#include <cmath>
#include <chrono>

#include <boost/program_options.hpp>

#include "pfabric.hpp"
#include "qcomp/TopologyBuilder.hpp"
#include "qcomp/SQLParser.hpp"
#include "qcomp/Plan.hpp"
#include "qcomp/QueryCompiler.hpp"

#include "TrajectoryDB.hpp"
#include "WebServer.hpp"
#include "PrefixSpan.hpp"

using namespace pfabric;
namespace po = boost::program_options;

extern double haversine_distance(double latitude1, double longitude1, double latitude2,
                          double longitude2);
extern void queryLoop(PFabricContext& ctx);

// uid: string, longitude: string, latitude: string
typedef TuplePtr<std::string, std::string, std::string> InpTuplePtr;

// uid: uint, longitude: double, latitude: double, ts: Timestamp
typedef TuplePtr<uint_t, double, double, Timestamp> TrackpointPtr;

// uid: uint, landmarkid: uint, landmark_longitude: double, landmark: double, latitude: double, ts: Timestamp
typedef TuplePtr<uint_t, uint_t, double, double, Timestamp> WaypointPtr;

// aggregate for #visits per landmark: landmark_id: uint, count(): uint
typedef Aggregator2<WaypointPtr, AggrIdentity<uint_t>, 1, AggrCount<uint_t, uint_t>, 0> VisitsAggregator;

// frequent trajectory
typedef TuplePtr<Pattern> PatternPtr;
typedef BatchPtr<Pattern> FreqTrajectoryBatch;

TrackpointPtr extractTrackpoint(InpTuplePtr tp, bool) {
  uint_t uid = 0;
  double lat = 0.0, lon = 0.0;
  if (get<0>(tp).substr(0, 4) == "uid=")
    uid = std::stoi(get<0>(tp).substr(4));
  if (get<1>(tp).substr(0, 4) == "lat=")
    lat = std::stod(get<1>(tp).substr(4));
  if (get<2>(tp).substr(0, 4) == "lon=")
    lon = std::stod(get<2>(tp).substr(4));
  return makeTuplePtr(uid, lat, lon, TimestampHelper::timestampFromCurrentTime() );
}

#if 1
PatternPtr userTrackToPattern(UserTrack tp, bool) {
  auto trackVec = get<1>(tp);
  std::vector<int> pvec(trackVec.size());
  for (std::size_t i = 0; i < trackVec.size(); i++)
    pvec[i] = trackVec[i].landmarkID;
  return makeTuplePtr(Pattern(pvec));
}

FreqTrajectoryBatch findFrequentTrajectories(BatchPtr<TuplePtr<Pattern>> batchPtr, bool) {
  //  batchPtr = TuplePtr<std::vector<std::pair<TuplePtr<Pattern>, bool>>>
  PrefixSpan::PatternList trajectories;
  auto patterns = get<0>(batchPtr); // patterns = std::vector<std::pair<TuplePtr<Pattern>, bool>>
  for (auto& pair : patterns) { // pairs = std::pair<TuplePtr<Pattern>, bool>
    trajectories.push_back(get<0>(pair.first));
  }

  PrefixSpan pfxSpan(2);
  auto res = PrefixSpan::suppressSubPatterns(pfxSpan.mineFreqPatterns(trajectories));
  // construct a batch from res
  std::vector<std::pair<Pattern, bool>> buf;
  for (auto& pattern : res) {
    buf.push_back(std::make_pair(pattern, false));
  }
  return makeTuplePtr(std::move(buf));
}
#endif

WaypointPtr findClosestWaypoint(std::shared_ptr<Table<Landmark::element_type, uint_t>> landmarksTable, 
    TrackpointPtr tp) {
  // waypoint to trackpoint
  auto iter = landmarksTable->select();
  double maxDistance = std::numeric_limits<double>::max();
  Landmark closestTrackpoint;
  for (; iter.isValid(); iter++) {
    double dist = haversine_distance(get<1>(tp), get<2>(tp), get<1>(*iter), get<2>(*iter));
    // std::cout << "distance to: " << get<3>(*iter) << ": " << dist << std::endl;
    if (dist < maxDistance) {
      maxDistance = dist;
      closestTrackpoint = *iter;
    }
  }
  return makeTuplePtr(get<0>(tp), get<0>(closestTrackpoint), 
                      get<1>(closestTrackpoint), get<2>(closestTrackpoint), get<3>(tp));
}

int main(int argc, char **argv) {
  po::options_description description("TrackerServer Usage");

  /* --- handle program options --- */
  description.add_options()
    ("help,h", "Display this help message")
    ("root,r", po::value<string>()->default_value("."), "Root directory for config, web, and database tables")
    ("import,i", "CSV file containing import data")
    ("query,q", "Start interactive query shell")
    ("version,v", "Display the version number");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << description << '\n';
    exit(-1);
  }

  if (vm.count("version")) {
    std::cout << "TrackerServer version 0.1\n";
    exit(-1);
  }

  std::string importFile = "";
  if (vm.count("import")) {
    importFile = vm["import"].as<std::string>();
  }

  bool allowQueries = false;
  if (vm.count("query")) {
    allowQueries = true;
  }

  std::string rootDir = ".";
  if (vm.count("root")) {
    rootDir = vm["root"].as<std::string>();
  }

  /* ----------------------------------------------------------------- */

  PFabricContext ctx;
  
  /* --- Create the necessary tables --- */
  createTables(ctx, importFile);

  auto visitsTable = ctx.getTable<Visit::element_type, uint_t>("visits");
  auto landmarksTable = ctx.getTable<Landmark::element_type, uint_t>("landmarks");
  auto tracksTable = ctx.getTable<UserTrack::element_type, uint_t>("user_tracks");

  /* --- Topology #1: Receive user positions via REST, store them in the user_tracks table and 
   *     update the visits table --- */
  auto t1 = ctx.createTopology();
  auto s = t1->newStreamFromREST(8099, "^/track$", RESTSource::POST_METHOD)
    .extract<InpTuplePtr>('&')
    .map<TrackpointPtr>(extractTrackpoint)
    .assignTimestamps([](auto tp) { return get<3>(tp); })
    .slidingWindow(WindowParams::RangeWindow, 6000) // 60 secs
    .map<WaypointPtr>([&](auto tp, bool) -> WaypointPtr {
      return findClosestWaypoint(landmarksTable, tp);
    });

  // store the waypoint in the user_tracks table
  auto s0 = s.keyBy<0, uint_t>()
    .updateTable<UserTrack, uint_t>(tracksTable, 
        [](WaypointPtr tp, bool outdated, UserTrack::element_type& rec) -> bool {
          get<1>(rec).push_back(Trackpoint(get<4>(tp), get<1>(tp)));
          return true;
        },
        [](WaypointPtr tp) -> UserTrack::element_type {
          return UserTrack::element_type(get<0>(tp), 
              std::vector<Trackpoint>({ Trackpoint(get<4>(tp), get<1>(tp)) }));
        }
    );

  // calculate the number of visits per landmark and store
  auto s1 = s.keyBy<1, uint_t>() // LandmarkID
    .groupBy<VisitsAggregator, uint_t>() // Number of visits per LandmarkID
    .map<Visit>([&](auto tp, bool) -> Visit {
      return makeTuplePtr(get<0>(tp), get<1>(tp));
    })
    .keyBy<0, uint_t>() // LandmarkID
    .toTable<uint_t>(visitsTable);
  
//  auto s2 = s.print(std::cout);

  t1->start();

#if 1
  /* --- Topology #2: Every 60 seconds report the visits data --- */
  auto t2 = ctx.createTopology();
  auto d = t2->selectFromTable<Visit, uint_t>(visitsTable)
    .print(std::cout);

  using namespace std::chrono_literals;
  t2->runEvery(1min);
#endif

#if 1
  /* --- Topology #3: Every 2 minutes compute the frequent trajectories --- */
  auto t3 = ctx.createTopology();
  auto ft = t3->selectFromTable<UserTrack, uint_t>(tracksTable)
    // map UserTrack -> Pattern
    .map<PatternPtr>(userTrackToPattern)
    .batch()
    .map<FreqTrajectoryBatch>(findFrequentTrajectories)
    .notify([](auto tp, bool outdated) {
      // tp = TuplePtr<std::vector<std::pair<InputStreamElement, bool>>>
      auto& vec = get<0>(tp);
      for (auto& elem : vec) {
        std::cout << elem.first << std::endl;
      }
    });

  t3->runEvery(2min);
#endif

  /* --- Start the internal Web server for serving files --- */
  HttpServer server;
  server.config.port = 8080;
  auto webThread = runWebServer(server, rootDir + "/web");

  if (allowQueries) {
    /* --- Interactive query loop --- */
    queryLoop(ctx);
  }

  t1->wait();
}
