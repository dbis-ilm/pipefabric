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
#ifndef FromTable_hpp_
#define FromTable_hpp_

#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <condition_variable>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"
#include "table/Table.hpp"

namespace pfabric {

  /**
   * @brief A FromTable operator creates a stream of elements from updates
   * on a relational table.
   *
   * The FromTable operator is an operator acting like a trigger which constructs
   * a stream of tuples from updates on a given relational table.
   *
   * @tparam StreamElement
   *    the data stream element type which shall be retrieve from the table
   * @tparam KeyType
   *    the data type of the key for identifying tuples in the table
   */
  template<typename StreamElement, typename KeyType = DefaultKeyType>
  class FromTable : public DataSource<StreamElement> {
  public:
    typedef std::shared_ptr<Table<StreamElement, KeyType>> TablePtr;

    PFABRIC_SOURCE_TYPEDEFS(StreamElement);


    /**
     * Create a new FromTable operator that registers with the given table @c tbl
     * and creates a stream of tuples from updates.
     *
     * @param tbl the table that is monitored
     * @param mode the notification mode for updates
     */
    FromTable(TablePtr tbl, TableParams::NotificationMode mode = TableParams::Immediate) :
      mInterrupted(false)  {
        tbl->registerObserver([this](const StreamElement& data, TableParams::ModificationMode m) {
          tableCallback(data, m);
        }, mode);
        mProducerThread = std::thread(&FromTable<StreamElement, KeyType>::producer, this);
    }

    /**
     * Deallocates all resources.
     */
    ~FromTable() {
      mInterrupted = true;
      {
        std::unique_lock<std::mutex> lock(mMtx);
        mCondVar.notify_one();
      }
      if (mProducerThread.joinable())
        mProducerThread.join();

    }

  protected:
    /**
     * A callback method that is registered with the table and called on
     * each update.
     *
     * @param data the actual tuple from the table
     * @param mode the modification mode of the table
     */
    void tableCallback(const StreamElement& data, TableParams::ModificationMode mode) {
      std::unique_lock<std::mutex> lock(mMtx);
      mQueue.push_back({ data, mode == TableParams::Insert});
      mCondVar.notify_one();
    }

    /**
     * Wait for tuples in the queue, remove them, and publish them as
     * stream element. This method is executed by a separate thread.
     */
    void producer() {
      while (!mInterrupted) {
        std::unique_lock<std::mutex> lock(mMtx);
        mCondVar.wait(lock, [&](){ return mInterrupted || !mQueue.empty(); });
        while (!mQueue.empty()) {
          auto tpair = mQueue.front();
          mQueue.pop_front();
          this->getOutputDataChannel().publish(tpair.first, tpair.second);
        }
      }
    }

    std::list<std::pair<StreamElement, bool>> mQueue; //< a queue of tuples to be published
    std::mutex mMtx;                                  //< mutex for accessing the queue
    std::condition_variable mCondVar;                 //< condition variable for waking up the producer
    bool mInterrupted;                                //< flag for interrupting the producer thread
    std::thread mProducerThread;                      //< the thread running the producer method
                                                      //< to publish tuples
  };

}

#endif
