/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
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
#include "TriggerNotifier.hpp"

#include <thread>


using namespace pfabric;

TriggerNotifier::TriggerNotifier(NotifierCallback::slot_type const& cb, unsigned int ti) :
  mInterrupted(new bool(false)), mTriggerInterval(ti) {
	mCallback.connect(cb);
	mThread = std::make_unique< std::thread >( (std::ref(*this) ) );
}

TriggerNotifier::~TriggerNotifier() {
  if (mThread) {
    // inform thread about stopping ...
    *mInterrupted = true;
    // and wait ...
    mThread->join();
  }
}

void TriggerNotifier::operator()() {
  // loop until the interrupted flag is set to true
  auto seconds = std::chrono::seconds(mTriggerInterval);

  while(!(*mInterrupted)) {
    // let's wait ...
    std::this_thread::sleep_for(seconds);
    // invoke the callback
		mCallback();
  }
}
