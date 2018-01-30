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

/*
 * Timer.hpp
 *
 *  Created on: Jan 5, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_UTILITIES_TIMER_HPP_
#define LIBCPP_UTILITIES_TIMER_HPP_

#include "ConvertTimeDurationToDouble.hpp"

#include <chrono>


namespace ns_utilities {


/**
 * @brief A simple timer class.
 *
 * This utility class can be used for profiling code segments. It is built based on top of the
 * @c chrono standard library. A @c Timer instance can be started and stopped to measure execution
 * time of the code between those points (wall time). Further, it allows to extract the execution
 * time in several time granularities, ranging from nanoseconds to hours.
 *
 * @note This timer class is intended for use within a single thread. Its methods are neither
 *       thread-safe, nor guaranteed to be accurate for measurements of parallel execution paths.
 *
 * @tparam ClockImpl
 *     the underlying clock used by the timer, defaults to the system's high resolution clock
 */
template<
	class ClockImpl = std::chrono::high_resolution_clock
>
class Timer {
private:

	/// a point in time used for calculating timer durations
	typedef std::chrono::time_point< ClockImpl > TimePoint;

	/**
	 * @brief The current state of the timer.
	 */
	enum State {
		STOPPED,         /**< the timer is not running *///!< STOPPED
		STARTED          /**< the timer is running */    //!< STARTED
	};


public:

	/// a type used to represent the time duration the timer ran
	typedef typename ClockImpl::duration Duration;

	/**
	 * @brief Create a new timer instance.
	 *
	 * The new timer instance is initialized to the @c STOPPED state.
	 */
	Timer()
		: mState( STOPPED ), mStart( Duration::zero() ), mEnd( Duration::zero() ) {

	}


	/**
	 * @brief Start the timer.
	 *
	 * This method throws an exception if the timer is already running.
	 */
	void start() {
		if( !isRunning() ) {
			mState = STARTED;
			mStart = ClockImpl::now();
		}
		else {
			throw "error: starting an already running timer";
		}
	}

	/**
	 * @brief Stop the timer.
	 *
	 * This method throws an exception if the timer is not running yet.
	 */
	void stop() {
		if( isRunning() ) {
			mEnd = ClockImpl::now();
			mState = STOPPED;
		}
		else {
			throw "error: stopping a non-running timer";
		}
	}

	/**
	 * @brief Check if the timer instance is currently running.
	 *
	 * @return @c true if the timer instance is running
	 *         @c false otherwise
	 */
	bool isRunning() const {
		return mState == STARTED;
	}


	/**
	 * @brief Get the elapsed time of the timer as @c Duration.
	 *
	 * @return the elapsed time duration as difference between
	 *         the timer's end and start time points if the timer is stopped,
	 *         the current time and timer's start time point if the timer is running
	 */
	Duration getElapsedTime() const {
		return ( isRunning() ? ClockImpl::now() : mEnd ) - mStart;
	}

	/**
	 * @brief Get the elapsed time in hours.
	 * @see getElapsedTime()
	 *
	 * @return the elapsed time in hours
	 */
	double getElapsedHours() const {
		return convertTimeDurationToDouble< Duration, std::chrono::hours >( getElapsedTime() );
	}

	/**
	 * @brief Get the elapsed time in minutes.
	 * @see getElapsedTime()
	 *
	 * @return the elapsed time in minutes
	 */
	double getElapsedMinutes() const {
		return convertTimeDurationToDouble< Duration, std::chrono::minutes >( getElapsedTime() );
	}

	/**
	 * @brief Get the elapsed time in seconds.
	 * @see getElapsedTime()
	 *
	 * @return the elapsed time in seconds
	 */
	double getElapsedSeconds() const {
		return convertTimeDurationToDouble< Duration, std::chrono::seconds >( getElapsedTime() );
	}

	/**
	 * @brief Get the elapsed time in milliseconds.
	 * @see getElapsedTime()
	 *
	 * @return the elapsed time in milliseconds
	 */
	double getElapsedMilliSeconds() const {
		return convertTimeDurationToDouble< Duration, std::chrono::milliseconds >( getElapsedTime() );
	}

	/**
	 * @brief Get the elapsed time in microseconds.
	 * @see getElapsedTime()
	 *
	 * @return the elapsed time in microseconds
	 */
	double getElapsedMicroSeconds() const {
		return convertTimeDurationToDouble< Duration, std::chrono::microseconds >( getElapsedTime() );
	}

	/**
	 * @brief Get the elapsed time in nanoseconds.
	 * @see getElapsedTime()
	 *
	 * @return the elapsed time in nanoseconds
	 */
	double getElapsedNanoSeconds() const {
		return convertTimeDurationToDouble< Duration, std::chrono::nanoseconds >( getElapsedTime() );
	}

private:

	State mState;     /**< the current state of the timer */
	TimePoint mStart; /**< the point in time the timer was started */
	TimePoint mEnd;   /**< the point in time the timer was stopped */
};


} /* end namespace ns_utilities */

#endif /* LIBCPP_UTILITIES_TIMER_HPP_ */
