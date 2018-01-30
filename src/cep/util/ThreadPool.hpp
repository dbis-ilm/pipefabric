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

#ifndef ThreadPool_hpp_
#define ThreadPool_hpp_

#include <boost/thread.hpp>
#include <boost/phoenix.hpp>
#include <boost/optional.hpp>

class ThreadPool {

public:
	typedef boost::function<void()> job;
	ThreadPool() :
			shutdown(false) {
		for (unsigned i = 0; i < boost::thread::hardware_concurrency(); ++i)
			pool.create_thread(boost::bind(worker_thread, boost::ref(*this)));
	}

	void enqueue(job job) {
		boost::lock_guard<boost::mutex> lk(mx);
		queue.push_back(job);

		cv.notify_one();
	}

	boost::optional<job> dequeue() {
		boost::unique_lock<boost::mutex> lk(mx);
		namespace phx = boost::phoenix;

		cv.wait(lk, phx::ref(shutdown) || !phx::empty(phx::ref(queue)));

		if (queue.empty())
			return boost::none;

		job job = queue.front();
		queue.pop_front();

		return job;
	}

	~ThreadPool() {
		shutdown = true;
		{
			boost::lock_guard<boost::mutex> lk(mx);
			cv.notify_all();
		}

		pool.join_all();
	}
private:
	boost::mutex mx;
	boost::condition_variable cv;
	std::deque<job> queue;

	boost::thread_group pool;

	boost::atomic_bool shutdown;
	static void worker_thread(ThreadPool& q) {
		while (boost::optional<job> job = q.dequeue())
			(*job)();
	}

};

#endif /*  ThreadPool_hpp_ */
