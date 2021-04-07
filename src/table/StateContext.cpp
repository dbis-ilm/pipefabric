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

#include "table/StateContext.hpp"
#include <iostream>

namespace pfabric {

/*==========================================================================*
 * Helper                                                                   *
 *==========================================================================*/

/** @brief Returns the position of the first 0 in v (binary, from right to
 *         left). */
uint8_t getFreePos(const uint64_t v) {
    static constexpr uint8_t tab64[64] = {
        63, 0, 58, 1, 59, 47, 53, 2,
        60, 39, 48, 27, 54, 33, 42, 3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22, 4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16, 9, 12,
        44, 24, 15, 8, 23, 7, 6, 5};

    /* Valid result is between 0 and 63
     * 64 means no free position */
    if (v == UINT64_MAX) return 64;
    /* Applying deBruijn hash function + lookup */
    return tab64[((uint64_t) ((~v & -~v) * 0x07EDD5E59A4E28C2)) >> 58];
}

/** @brief Returns and atomically sets the position of the first 0 in v (binary,
 *         from right to left). */
uint8_t getSetFreePos(std::atomic<std::uint64_t> &v) {
  uint8_t pos;
  uint64_t expected = v.load(std::memory_order_relaxed);
  do {
    pos = getFreePos(expected); //TODO: catch if no free position
  } while(!v.compare_exchange_weak(
        expected,
        expected | (1ULL << pos), //< set bit at pos
        std::memory_order_relaxed));
  return pos;
}

/** @brief Atomically unsets the bit at position pos in v */
void unsetPos(std::atomic<std::uint64_t> &v, const uint8_t pos) {
  uint64_t expected = v.load(std::memory_order_relaxed);
  while(!v.compare_exchange_weak(
        expected,
        expected & ~(1ULL <<pos), //< unset bit at pos
        std::memory_order_relaxed));
}

/* taken from https://stackoverflow.com/a/12996028 */
unsigned int hashMe(unsigned int x) {
  ++x;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

} /* end namespace pfabric */
