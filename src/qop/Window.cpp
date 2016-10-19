/*
 * Copyright (c) 2014 The PipeFabric team,
 *                    All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
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
/*
  while(!(*mInterrupted)) {
    // let's wait some time
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.sec += mEvictInterval;
    boost::thread::sleep(xt);
    // and finally invoke the eviction function
    if(!(*mInterrupted)) {
		    mEvictFun();
    }
  }
  */
}
