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
#ifndef Queue_hpp_
#define Queue_hpp_

#include <thread>

#include <boost/signals2.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {
	/**
	 * @brief Helper class for dequeing tuples from the queue.
	 *
	 * DequeueNotifier is a helper class for the Queue operator. It invokes
	 * a given callback (implemented by a boost::signal) of the associated operator.
	 */
	class DequeueNotifier {
	public:
		/**
		 * Typedef for the notifier callback.
		 */
		typedef boost::signals2::signal<void (DequeueNotifier& sender)> DequeueSignal;

		/**
		 * Creates a new notifier object.
		 *
		 * \param cb the callback which is invoked periodically.
		 */
		DequeueNotifier(DequeueSignal::slot_type const& cb) : mInterrupted(new bool(false)) {
			mDequeueCallback.connect(cb);
			mThread = std::make_unique< std::thread >( (std::ref(*this) ) );
		}

		/**
		 * Destructor for deallocating resources.
		 */
		~DequeueNotifier() {
			if (mThread) {
				// inform the thread about stopping
				*mInterrupted = true;
				// and wait until it has stopped
				mThread->join();
			}
		}

		/**
		 * Checks the interrupted flag.
		 *
		 * \return true if the notifier is interrupted.
		 */
		bool isInterrupted() { return *mInterrupted; }

		/**
		 * The method doing the work of the notifier. It is called by the
		 * std::thread class.
		 */
		void operator()() {
			while (! (*mInterrupted)) {
				mDequeueCallback(*this);
			}
		}

	private:
		std::unique_ptr<std::thread> mThread;              //< the notifier thread
		std::shared_ptr<bool> mInterrupted;  //< flag for cancelling the thread (true if the thread can be stopped)
		                                     //< note it has to be a shared pointer, because the object is copied during creating the thread.
		DequeueSignal mDequeueCallback;      //< the callback which is invoked
	};

	/**
	 * @brief an operator for decouping tuple producer and consumer.
	 *
	 * The Queue operator is used for decoupling tuple producer and consumer by inserting a tuple queue
	 * between two operators and creating a separate consumer thread that waits for incoming tuples and forwards
	 * them to the subscriber.
	 */
	template<class StreamElement>
	class Queue : public UnaryTransform<StreamElement, StreamElement> { // use default unary transform
	public:
	  PFABRIC_UNARY_TRANSFORM_TYPEDEFS(StreamElement, StreamElement)

		/**
		 * Creates a new instance of the operator.
		 */
		Queue() : mNotifier(new DequeueNotifier(boost::bind(&Queue::dequeueTuple, this, _1))) {
		}

		/**
		 * Frees all allocated resources, i.e. deletes the notifier thread.
		 */
		~Queue() {
			// delete mNotifier;
		}

		/**
		 * @brief Bind the callback for the data channel.
		 */
		BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, Queue, processDataElement );

		/**
		 * @brief Bind the callback for the punctuation channel.
		 */
		BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, Queue, processPunctuation );

		/**
		 * This method is invoked when a punctuation arrives. It simply forwards the punctuation
		 * to the subscribers as soon as the queue is empty.
		 *
		 * \param punctuation the incoming punctuation tuple
		 */
		void processPunctuation( const PunctuationPtr& punctuation ) {
			while (!mQueue.empty()) ;
			this->getOutputPunctuationChannel().publish(punctuation);
		}
		/**
		 * Implements the callback invoked by the notifier thread. It reads the next tuple from the queue and
		 * sends it to the publishers.
		 *
		 * \param sender a reference to the notifier object
		 */
		void dequeueTuple(DequeueNotifier& sender) {
			StreamElement tp;
			while (!mQueue.pop(tp) && !sender.isInterrupted()) ;
			// TODO: is this still necessary??
		//	StreamElement newTp(tp);
			this->getOutputDataChannel().publish(tp, false);
		}

	/**
	 * This method is invoked when a tuple arrives from the publisher. It inserts the incoming tuple
	 * into the queue.
	 *
	 * \param data the incoming tuple
	 * \param outdated indicates whether the tuple is new or invalidated now (outdated == true)
	 */
	void processDataElement( const StreamElement& data, const bool outdated ) {
		while (!mQueue.push(data));
	}

private:
		boost::lockfree::spsc_queue<StreamElement, boost::lockfree::capacity<65535> > mQueue; //< a queue acting as concurrent buffer for tuples
		std::unique_ptr<DequeueNotifier> mNotifier;    //< the notifier object which triggers the computation of aggregates periodically

};
}

#endif
