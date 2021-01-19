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

#include "UniqueNameGenerator.hpp"
#include "fmt/format.h"

using namespace pfabric;

UniqueNameGenerator* UniqueNameGenerator::theInstance = nullptr;
std::mutex UniqueNameGenerator::mCreateMtx;

UniqueNameGenerator::UniqueNameGenerator() : mCounter(0) {
}

UniqueNameGenerator* UniqueNameGenerator::instance() {
  std::lock_guard<std::mutex> lock(mCreateMtx);
  if (theInstance == nullptr) 
    theInstance = new UniqueNameGenerator();
  return theInstance;
}

std::string UniqueNameGenerator::uniqueName(const std::string& prefix) {
  std::lock_guard<std::mutex> lock(mNextMtx);
  mCounter++;
  return fmt::format("{0}_{1}", prefix, mCounter);
}
