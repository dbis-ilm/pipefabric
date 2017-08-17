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
#ifndef SlidingWindow_hpp_
#define SlidingWindow_hpp_

#include "Window.hpp"

#include <boost/core/ignore_unused.hpp>


namespace pfabric {

  /**
   * @brief  SlidingWindow implements a sliding window operator.
   *
   * SlidingWindow represents a sliding window operator which invalidates tuples
   * based on a given time interval. Each incoming tuple is forwarded to the
   * output pipe and after the specified time interval (the window size)
   * a corresponding outdated tuple is produced indicating the invalidation of
   * the original tuple. Specifying an eviction interval != 0 allows the
   * check the window periodically instead of evicting tuples only if
   * new tuples arrive.
   *
   * @tparam StreamElement
   *    the data stream element type kept in the window
   */
  template<
  typename StreamElement
  >
  class SlidingWindow : public Window< StreamElement > {
  private:

    /// the window base class
    typedef Window< StreamElement > WindowBase;

    PFABRIC_BASE_TYPEDEFS(WindowBase, StreamElement);

  public:

    /**
     * @brief Create a new sliding window operator instance with the given parameters.
     *
     * Create a new sliding window operator of a given window type with a timestamp
     * extractor function. This constructor should be mainly used with time-based
     * windows (WindowParams::RangeWindow).
     *
     * @param func a function for extracting the timestamp value from the stream element
     * @param wt the type of the window (range or row)
     * @param sz the window size (seconds or number of tuples)
     * @param windowFunc optional function for modifying incoming tuples
     * @param ei ei the eviction interval, i.e., time for triggering the eviction (in milliseconds)
     */
    SlidingWindow(typename Window<StreamElement>::TimestampExtractorFunc func,
                  const WindowParams::WinType& wt,
                  const unsigned int sz,
                  typename Window<StreamElement>::WindowOpFunc windowFunc = nullptr,
                  const unsigned int ei = 0) :
    WindowBase(func, wt, sz, windowFunc, ei ) {
      if (ei == 0) {
        // sliding window where the incoming tuple evicts outdated tuples
        this->mEvictFun = std::bind( this->mWinType == WindowParams::RangeWindow ?
                                      &SlidingWindow::evictByTime : &SlidingWindow::evictByCount, this
                                      );
      }
      else {
        // sliding window, but we need a thread for evicting tuples
        WindowParams::EvictionFunc efun = boost::bind( &SlidingWindow::evictByTime, this );
        this->mEvictThread = std::make_unique< EvictionNotifier >( this->mEvictInterval, efun );
      }
    }

    /**
     * @brief Create a new sliding window operator instance with the given parameters.
     *
     * Create a new sliding window operator of a given window . This constructor
     * should be mainly used with row-based windows (WindowParams::RowWindow).
     *
     * @param wt the type of the window (range or row)
     * @param sz the window size (seconds or number of tuples)
     * @param windowFunc optional function for modifying incoming tuples
     * @param ei ei the eviction interval, i.e., time for triggering the eviction (in milliseconds)
     */
    SlidingWindow(const WindowParams::WinType& wt,
                  const unsigned int sz,
                  typename Window<StreamElement>::WindowOpFunc windowFunc = nullptr,
                  const unsigned int ei = 0) :
    WindowBase(wt, sz, windowFunc, ei ) {
      if (ei == 0) {
        // sliding window where the incoming tuple evicts outdated tuples
        this->mEvictFun = std::bind( this->mWinType == WindowParams::RangeWindow ?
                                      &SlidingWindow::evictByTime : &SlidingWindow::evictByCount, this
                                      );
      }
      else {
        // sliding window, but we need a thread for evicting tuples
        WindowParams::EvictionFunc efun = boost::bind( &SlidingWindow::evictByTime, this );
        this->mEvictThread = std::make_unique< EvictionNotifier >( this->mEvictInterval, efun );
      }
    }

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, SlidingWindow, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, SlidingWindow, processPunctuation );


  private:

    /**
     * @brief This method is invoked when a punctuation arrives.
     *
     * It simply ignores the punctuation because a window generates its own punctuations.
     *
     * @param[in] punctuation
     *    the incoming punctuation tuple
     */
    void processPunctuation( const PunctuationPtr& punctuation ) {
      boost::ignore_unused( punctuation );
    }

    /**
     * @brief This method is invoked when a tuple arrives from the publisher.
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement( const StreamElement& data, const bool outdated ) {
      if( outdated == true ) {
        // not sure if this is really necessary
        this->getOutputDataChannel().publish(data, outdated);
      } else {
        // if function available
        if(this->mWindowOpFunc != nullptr) {
        // insert the tuple into buffer
          { //necessary for lock scope!
            std::lock_guard<std::mutex> guard(this->mMtx);
            this->mTupleBuf.push_back(data);
            this->mCurrSize++;
          }

          // check for outdated tuples
          if (!this->mEvictThread) {
            this->mEvictFun();
          }
          // apply the window function - we do this after the window was updated
          auto res = this->mWindowOpFunc(this->mTupleBuf.begin(),
           this->mTupleBuf.end(), data);

          // finally, forward the incoming tuple
          this->getOutputDataChannel().publish(res, outdated);

        } else {
          // insert the tuple into buffer
          { //necessary for lock scope!
            std::lock_guard<std::mutex> guard(this->mMtx);
            this->mTupleBuf.push_back(data);
            this->mCurrSize++;
          }

          // check for outdated tuples
          if (!this->mEvictThread) {
            this->mEvictFun();
          }

          // finally, forward the incoming tuple
          this->getOutputDataChannel().publish(data, outdated);
        }
      }
    }


    /**
     * Implements an eviction strategy for RowWindow, i.e. a tuple is
     * outdated as soon as the addition of a new tuple to the
     * window exceeds the given window size.
     */
    void evictByCount() {
      std::lock_guard<std::mutex> guard(this->mMtx);
      // as long as we have too many tuples ...
      while (this->mCurrSize > this->mWinSize) {
        const auto tup = this->mTupleBuf.front();
        // let's get rid of it ...
        this->mTupleBuf.pop_front();
        this->mCurrSize--;
        // and publish it as outdated tuple
        this->getOutputDataChannel().publish(tup, true);
      }
    }

    /**
     * Implements an eviction strategy for RangeWindow, i.e. a tuple is
     * outdated as soon as the time difference between this tuple and the
     * most recent one in the window exceeds the given window size.
     */
    void evictByTime() {
      std::lock_guard<std::mutex> guard(this->mMtx);
      const auto& lastWindowElement = this->mTupleBuf.back();
      const Timestamp lastTupleTime = this->mTimestampExtractor( lastWindowElement );

      /*
       * It may happen that the timestamp of a tuple is less than window time, e.g. if we work with artificial timestamps like 0, 1, ...
       * In this case, accepted_time could be less than 0. We check this before to avoid overflows.
       */
      if( lastTupleTime >= this->mDiffTime ) {
        const Timestamp accepted_time = lastTupleTime - this->mDiffTime;
        bool finished = false;

        // as long as we have tuples which are now outside the valid time window ...
        while (!finished && !this->mTupleBuf.empty()) {
          const auto tup = this->mTupleBuf.front();
          if( this->mTimestampExtractor( tup ) < accepted_time ) {
            // let's get rid of it ...
            this->mTupleBuf.pop_front();
            this->mCurrSize--;
            // and publish it as outdated tuple
            this->getOutputDataChannel().publish(tup, true);
          }
          else {
            finished = true;
          }
        }
      }
    }
  };

}

#endif
