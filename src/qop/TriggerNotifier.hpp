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

#ifndef TriggerNotifier_hpp_
#define TriggerNotifier_hpp_

// #include <boost/thread.hpp>

#include <thread>

#include <boost/signals2.hpp>

#include "libcpp/types/types.hpp"


namespace pfabric {
  /**
   * @brief Helper class for operators which produce results periodically
   *
   * TriggerNotifier is a helper class for operators which produce results
   * periodically, e.g. aggregations. It invokes a given callback (implemented by
   * a boost::signal) of the associated operator periodically.
   */
  class TriggerNotifier {
  public:
    /**
     * Typedef for the notifier callback.
     */
    typedef boost::signals2::signal<void ()> NotifierCallback;

    /**
     * Create a new notifier object.
     *
     * @param cb the callback which is invoked periodically.
     * @param slen the time interval for notifications.
     */
    TriggerNotifier(NotifierCallback::slot_type const& cb, unsigned int slen);

    /**
     * Destructor for deallocating resources.
     */
    ~TriggerNotifier();

    /**
     * The method doing the work of the notifier. It is called by the
     * boost::thread class.
     */
    void operator()();

  private:
    typedef std::unique_ptr< std::thread > ThreadPtr;
    std::shared_ptr<bool> mInterrupted; //< flag for cancelling the thread (true if the thread can be stopped)
                                        //<  note it has to be a shared pointer, because the object is copied //< during creating the thread.
    ThreadPtr mThread;                  //< the notifier thread
    NotifierCallback mCallback;	        //< the callback which is invoked
    unsigned int mTriggerInterval;      //< the time interval for notifications
  };
}

#endif
