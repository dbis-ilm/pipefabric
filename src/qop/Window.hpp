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
#ifndef Window_hpp_
#define Window_hpp_

#include <list>
#include <thread>
#include <mutex>
#include <boost/assert.hpp>

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

  class EvictionNotifier;

  struct WindowParams {
    /**
     * Typedef for a function implementing the specific eviction strategy.
     */
    typedef std::function<void()> EvictionFunc;

    /**
     * Literals for the supported types of windows.
     */
    enum WinType {
      InvalidWindow,        //< invalid window, shouldn't be used
      RangeWindow,          //< a window storing tuples valid for a time duration
      RowWindow             //< a window storing a maximum number of tuples
    };

  };

  /**
   * @brief An operator implementing a sliding or tumbling window on the
   * input data stream.
   *
   * The window operator keeps a portion of the stream on which other
   * operators such as join or aggregates can be applied. The window is
   * parameterized either by the number of tuples kept in the window or
   * the time interval during a tuple is valid (and kept in the window).
   * Note, that this class provides only an interface, the actual window
   * implementations are done in specific subclasses.
   *
   * @tparam StreamElement
   *    the data stream element type kept in the window
   */
  template<
  typename StreamElement
  >
  class Window :
  public UnaryTransform< StreamElement, StreamElement > // use default unary transform
  {
  public:
    typedef std::function<Timestamp(const StreamElement&)> TimestampExtractorFunc;

  protected:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement);

    /**
     * Creates a new window operator instance with the given parameters.
     *
     * @param func a function for extracting the timestamp value from the stream element
     * @param wt the type of the window (range or row)
     * @param sz the window size (seconds or number of tuples)
     * @param ei the eviction interval, i.e., time for triggering the eviction (in milliseconds)
     */
    Window(TimestampExtractorFunc func, const WindowParams::WinType& wt,
           const unsigned int sz, const unsigned int ei = 0) :
    mTimestampExtractor(func), mWinType(wt), mWinSize(sz), mEvictInterval(ei), mCurrSize(0) {
      mDiffTime = (mWinType == WindowParams::RangeWindow ?
                   boost::posix_time::seconds(mWinSize).total_microseconds() : 0
                   );
    }

    /**
     * @brief Create a new  window operator instance with the given parameters.
     *
     * Create a new sliding window operator of a given window . This constructor
     * should be mainly used with row-based windows (WindowParams::RowWindow).
     *
     * @param wt the type of the window (range or row)
     * @param sz the window size (seconds or number of tuples)
     * @param ei ei the eviction interval, i.e., time for triggering the eviction (in milliseconds)
     */
    Window(const WindowParams::WinType& wt,
           const unsigned int sz, const unsigned int ei = 0) :
    mWinType(wt), mWinSize(sz), mEvictInterval(ei), mCurrSize(0), mDiffTime(0) {
      BOOST_ASSERT_MSG(mWinType == WindowParams::RowWindow, "RowWindow requires timestamp extractor function.");
    }

    /// a list for stream elements in the windows
    typedef std::list< StreamElement > TupleList;

    typedef std::unique_ptr< EvictionNotifier > EvictionThread;

    TimestampExtractorFunc mTimestampExtractor; //< a function for extracting timestamps from a tuple
    WindowParams::WinType mWinType;             //< the type of window
    unsigned int mWinSize;                      //< the size of window (time or number of tuples)
    unsigned int mEvictInterval;                //< the slide length of window (time or number of tuples)
    TupleList mTupleBuf;                        //< the actual window buffer
    unsigned int mCurrSize;                     //< the current number of tuples in the window
    Timestamp mDiffTime;                        //< for time-based window the window size in
                                                //< number of microseconds
    WindowParams::EvictionFunc mEvictFun;       //< a function implementing the eviction policy
    EvictionThread mEvictThread;                //< the thread for running the eviction function
                                                //< (if the eviction interval > 0)
    mutable std::mutex mMtx;                    //< mutex for accessing the tuple buffer
  };

  /**
   * @brief Helper class for the window operator
   *
   * EvictionNotifier is a helper class for the window operator to
   * invoke the eviction function periodically.
   *
   * TODO: why not using TriggerNotifier instead???
   */
  class EvictionNotifier {
  public:
    /**
     * Create a new notifier object.
     *
     * @param ei the eviction interval, i.e., time for triggering the eviction (in milliseconds)
     * @param fun the eviction member function
     */
    EvictionNotifier(unsigned int ei, WindowParams::EvictionFunc& fun);

    /**
     * Destructor
     */
    ~EvictionNotifier();

    /**
     * The method doing the work of the notifier. It is called by the
     * boost::thread class.
     */
    void operator()();

  private:
    typedef std::shared_ptr< std::thread > ThreadPtr;
    std::shared_ptr<bool> mInterrupted;   //< flag for canceling the thread (true if the thread can be stopped)
                                          //< note it has to be a shared pointer, because the object is copied
                                          //< during creating the thread.
    ThreadPtr mThread;                    //< the notifier thread
    unsigned int mEvictInterval;          //< the time interval for notifications
    WindowParams::EvictionFunc mEvictFun; //< the eviction function we call periodically
  };

} /* end namespace pfabric */


#endif
