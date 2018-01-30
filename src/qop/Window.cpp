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

#include <thread>
#include <boost/version.hpp>

#include "Window.hpp"

#if BOOST_VERSION < 105000
#define TIME_UTC_ TIME_UTC
#endif

using namespace pfabric;

EvictionNotifier::EvictionNotifier(unsigned int ei, WindowParams::EvictionFunc& fun) :
mInterrupted(new bool(false)), mEvictInterval(ei), mEvictFun(fun) {
  mThread = std::make_shared< std::thread >( *this );
}

EvictionNotifier::~EvictionNotifier() {
  if (mThread) {
    // inform the thread about stopping
    *mInterrupted = true;
    // and wait until it has stopped
    mThread->join();
  }
}

void EvictionNotifier::operator()() {
  // loop until the interrupted flag is set to true
  auto seconds = std::chrono::seconds(mEvictInterval);

  while(!(*mInterrupted)) {
    // let's wait ...
    std::this_thread::sleep_for(seconds);
    // invoke the callback
    mEvictFun();
  }
}
