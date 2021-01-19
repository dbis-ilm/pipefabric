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

#ifndef TumblingWindow_hpp_
#define TumblingWindow_hpp_

#include "Window.hpp"


namespace pfabric {

  /**
   * @brief  TumblingWindow implements a tumbling window operator.
   *
   * TumblingWindow represents a window operator with tumbling window semantics
   * where all tuples are outdated and the window is started from scratch as
   * soon is the window size is exceeded.
   *
   * @tparam StreamElement
   *    the data stream element type kept in the window
   */
  template<
  typename StreamElement
  >
  class TumblingWindow : public Window< StreamElement > {
  private:

    /// the window base class
    typedef Window< StreamElement > WindowBase;

    PFABRIC_BASE_TYPEDEFS(WindowBase, StreamElement);

  public:

    /**
     * Create a new tumbling window operator instance with the given parameters.
     *
     * @param func a function for extracting the timestamp value from the stream element
     * @param wt the type of the window (range or row)
     * @param sz the window size (seconds or number of tuples)
     * @param windowFunc optional function for modifying incoming tuples
     */
    TumblingWindow(typename Window<StreamElement>::TimestampExtractorFunc func,
                   const WindowParams::WinType& wt,
                   const unsigned int sz,
                   typename Window<StreamElement>::WindowOpFunc windowFunc = nullptr) :
    WindowBase(func, wt, sz, windowFunc ) {
      setupEviction();
    }

    /**
     * Create a new tumbling window operator instance with the given parameters.
     *
     * @param func a function for extracting the timestamp value from the stream element
     * @param wt the type of the window (range or row)
     * @param sz the window size (as chrono duration or number of tuples)
     * @param windowFunc optional function for modifying incoming tuples
     */
    template<class Rep, class Period = std::ratio<1>>
    TumblingWindow(typename Window<StreamElement>::TimestampExtractorFunc func,
                   const WindowParams::WinType& wt,
                   const std::chrono::duration<Rep, Period> sz,
                   typename Window<StreamElement>::WindowOpFunc windowFunc = nullptr) :
    WindowBase(func, wt, sz, windowFunc ) {
      setupEviction();
    }

    /**
     * Create a new tumbling window operator instance with the given parameters.
     *
     * @param wt the type of the window (range or row)
     * @param sz the window size (seconds or number of tuples)
     * @param windowFunc optional function for modifying incoming tuples
     */
    TumblingWindow(const WindowParams::WinType& wt, const unsigned int sz,
    typename Window<StreamElement>::WindowOpFunc windowFunc = nullptr) :
    WindowBase(wt, sz, windowFunc ) {
      setupEviction();
    }

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, TumblingWindow, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, TumblingWindow, processPunctuation );


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
      // TODO same strategy as sliding window?

      if( outdated == true ) {
        // not sure if this is really necessary
        this->getOutputDataChannel().publish(data, outdated);
        return;
      }
      else {
        // if function available
        if(this->mWindowOpFunc != nullptr) {
          // insert the tuple into buffer
          { //necessary for lock scope!
            std::lock_guard<std::mutex> guard(this->mMtx);
            this->mTupleBuf.push_back(data);
            this->mCurrSize++;
          }

          // apply function on tuple
          auto res = this->mWindowOpFunc(this->mTupleBuf.begin(),
            this->mTupleBuf.end(), data);

        //forward the incoming tuple
          this->getOutputDataChannel().publish(res, outdated);

          // check for outdated tuples
          if (!this->mEvictThread) {
            this->mEvictFun();
          }

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

          //forward the incoming tuple
          this->getOutputDataChannel().publish(data, outdated);
        }
      }
    }

    /**
     * Sets up the eviction function.
     **/
    void setupEviction() {
      if( this->mWinType == WindowParams::RowWindow ) {
        this->mEvictFun = std::bind(&TumblingWindow::evictByCount, this);
      } else {
        this->mEvictFun = std::bind(&TumblingWindow::evictByTime, this);
      }
    }

    /**
     * Implements an eviction strategy for RowWindow, i.e. all tuples are
     * outdated as soon as the addition of a new tuple to the
     * window exceeds the given window size.
     */
    void evictByCount() {
      std::lock_guard<std::mutex> guard(this->mMtx);

      if (this->mCurrSize == boost::get<unsigned int>(this->mWinSize)) {
        for (auto it = this->mTupleBuf.begin(); it != this->mTupleBuf.end(); it++) {
          this->getOutputDataChannel().publish( *it, true );
        }

        this->mCurrSize = 0;
        this->mTupleBuf.clear();
        auto pp = std::make_shared< Punctuation >( Punctuation::WindowExpired );
        this->getOutputPunctuationChannel().publish( pp );
      }
    }

    /**
     * Implements an eviction strategy for RangeWindow, i.e. all tuples are
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
      if( lastTupleTime >= boost::get<Timestamp>(this->mWinSize) ) {
        const Timestamp accepted_time = lastTupleTime - boost::get<Timestamp>(this->mWinSize);
        const auto& tup = this->mTupleBuf.front();

        if( this->mTimestampExtractor( tup ) <= accepted_time ) {
          // we have inserted the most recent tuple already because we need its timestamp,
          // but we don't evict it yet
          const auto end = --(this->mTupleBuf.end());
          for (auto it = this->mTupleBuf.begin(); it != end; it++) {
            this->getOutputDataChannel().publish( *it, true );
          }

          this->mCurrSize = 1;
          this->mTupleBuf.erase( this->mTupleBuf.begin(), end );
          auto pp = std::make_shared< Punctuation >( Punctuation::WindowExpired );
          this->getOutputPunctuationChannel().publish( pp );
        }
      }
    }
  };

}


#endif
