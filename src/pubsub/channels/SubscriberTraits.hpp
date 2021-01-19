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

/*
 * SubscriberTraits.hpp
 *
 *  Created on: Jan 29, 2015
 *      Author: fbeier
 */

#ifndef SUBSCRIBERTRAITS_HPP_
#define SUBSCRIBERTRAITS_HPP_

#include "SubscriptionBase.hpp"


/**
 * @brief Traits class for as subscriber component.
 *
 * @tparam SubscriberImpl
 *    the subscriber implementation for which the traits are defined
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename SubscriberImpl
>
class SubscriberTraits {
public:
	//////   public types   //////

	/// the subscriber component whose traits are defined here
	typedef SubscriberImpl Subscriber;

	/// a weak reference to the shared subscriber instance
	typedef typename Subscriber::WeakRef WeakRef;

	/// a shared reference to the subscriber instance
	typedef typename Subscriber::SharedRef SharedRef;

	/// the slot type used as callback by the subscriber
	typedef typename Subscriber::Slot Slot;


	//////   public constants   //////


	//////   public interface   //////

	/**
	 * @brief Get a weak reference to a subscriber instance.
	 *
	 * @param[in] subscriber
	 *    the subscriber whose weak reference shall be returned
	 * @return a weak subscriber reference
	 */
	static WeakRef getWeakRef( Subscriber& subscriber ) {
		return subscriber.getWeakRef();
	}

	/**
	 * @brief Establish a connection between a specific publisher and a subscriber instance.
	 *
	 * @tparam Publisher
	 *    the publisher type
	 * @param[in] subscriber
	 *    the subscriber instance which registers for the data produced by the @c publisher
	 * @param[in] publisher
	 *    the publisher instance producing data elements
	 * @return a handle representing the new subscription
	 */
	template< typename Publisher >
	static SubscriptionPtr subscribe( Subscriber& subscriber, Publisher& publisher ) {
		return subscriber.subscribe( publisher );
	}

	/**
	 * @brief Establish a connection between a specific publisher and a subscriber instance.
	 *
	 * @tparam Publisher
	 *    the publisher type
	 * @param[in] publisher
	 *    the publisher instance producing data elements
	 * @param[in] subscriber
	 *    the subscriber instance which registers for the data produced by the @c publisher
	 * @return a handle representing the new subscription
	 */
	template< typename Publisher >
	static SubscriptionPtr subscribe( Publisher& publisher, Subscriber& subscriber ) {
		return subscriber.subscribe( publisher );
	}


	/**
	 * @brief Get the slot that is used as callback by the subscriber for handling
	 *        data that is produced by a publisher.
	 *
	 * @param[in] subscriber
	 *            the channel which requests the slot for a new subscription
	 * @return the callback for notifying the subscriber of new data elements
	 */
	static Slot getSlot( Subscriber& subscriber ) {
		return subscriber.getSlot();
	}

	/**
	 * @brief Register a new subscription at the subscriber.
	 *
	 * This bookkeeping method is invoked when a new subscription has been created.
	 * Subscribers should maintain a reference to all their subscriptions in order to keep them
	 * alive as long as the subscriber instance exists and close them when the latter is destroyed.
	 *
	 * @tparam Subscription
	 *    the actual subscription type
	 * @param[in] subscriber
	 *    the subscriber where the subscription shall be registered
	 * @param[in] subscription
	 *    the subscription to be registered at the subscriber
	 */
	template< typename Subscription >
	static void addSubscription( Subscriber& subscriber, Subscription& subscription ) {
		subscriber.addSubscription( subscription );
	}

	/**
	 * @brief Unregister a subscription from the subscriber.
	 *
	 * This bookkeeping method is invoked when a subscriber's subscription is closed.
	 * It will be used to update all references to the subscriber's subscriptions.
	 *
	 * @tparam Subscription
	 *    the actual subscription type
	 * @param[in] subscriber
	 *    the subscriber where the subscription shall be unregistered
	 * @param[in] subscription
	 *    the subscription to be unregistered at the subscriber
	 */
	template< typename Subscription >
	static void removeSubscription( Subscriber& subscriber, Subscription& subscription ) {
		subscriber.removeSubscription( subscription );
	}
};


#endif /* SUBSCRIBERTRAITS_HPP_ */
