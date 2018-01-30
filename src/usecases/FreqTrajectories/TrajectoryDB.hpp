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
