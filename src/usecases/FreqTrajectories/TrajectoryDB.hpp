#ifndef TrajectoryDB_hpp_
#define TrajectoryDB_hpp_

#include "pfabric.hpp"
#include <string>
#include <vector>

using namespace pfabric;

typedef unsigned int uint_t;

// pointID, description, count
/*
 * CREATE TABLE visits (pointID uint, count uint);
 */
typedef TuplePtr<uint_t, uint_t> Visit;

// pointID, latitude, longitude, description
/*
 * CREATE TABLE landmarks (pointID uint, longitude double, latitute double, description string);
 */
typedef TuplePtr<uint_t, double, double, std::string> Landmark;

struct Trackpoint {
  Timestamp ts;
  uint_t landmarkID;
  explicit Trackpoint(Timestamp t, uint_t id) : ts(t), landmarkID(id) {}
  explicit Trackpoint() {}
};

// userID, path
typedef TuplePtr<uint_t, std::vector<Trackpoint>> UserTrack;

void createTables(PFabricContext& ctx, const std::string& landmarkFile = "");

#endif
