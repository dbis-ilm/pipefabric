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
 * PublisherTraits.hpp
 *
 *  Created on: Jan 29, 2015
 *      Author: fbeier
 */

#ifndef PUBLISHERTRAITS_HPP_
#define PUBLISHERTRAITS_HPP_

#include "SubscriptionBase.hpp"


/**
 * @brief Traits class for as publisher component.
 *
 * @tparam PublisherImpl
 *    the publisher implementation for which the traits are defined
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename PublisherImpl
>
class PublisherTraits {
public:
	//////   public types   //////

	/// the publisher compontent whose traits are defined here
	typedef PublisherImpl Publisher;

	/// a weak reference to the shared publisher instance
	typedef typename Publisher::WeakRef WeakRef;

	/// a shared reference to the publisher instance
	typedef typename Publisher::SharedRef SharedRef;


	//////   public constants   //////


	//////   public interface   //////

	/**
	 * @brief Get a weak reference to a publisher instance.
	 *
	 * @param[in] publisher
	 *    the publisher whose weak reference shall be returned
	 * @return a weak publisher reference
	 */
	static WeakRef getWeakRef( Publisher& publisher ) {
		return publisher.getWeakRef();
	}


	/**
	 * @brief Publish some data elements through a publisher instance.
	 *
	 * This method will publish @c data elements to all subscribers that subscribed
	 * to a specific @c publisher instance.
	 *
	 * @tparam PublishedTypes
	 *    the list of data element types to be published
	 * @param[in] publisher
	 *    the publisher instance which shall publish the data
	 * @param[in] data
	 *    the data elements to be published
	 */
	template< typename... PublishedTypes >
	static void publish( Publisher& publisher, PublishedTypes&&... data ) {
		// TODO check that the types are convertible to the channel types
		publisher.publish( std::forward< PublishedTypes >( data )... );
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
	template< typename Subscriber >
	static SubscriptionPtr subscribe( Publisher& publisher, Subscriber& subscriber ) {
		return publisher.subscribe( subscriber );
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
	template< typename Subscriber >
	static SubscriptionPtr subscribe( Subscriber& subscriber, Publisher& publisher ) {
		return publisher.subscribe( subscriber );
	}


	/**
	 * @brief Close the connection between a publisher and a subscriber instance.
	 *
	 * This method will close the implementation-specific @c Connection that was created
	 * when a subscriber subscribed the @c publisher instance. These connection handles
	 * are wrapped in @c Subscriptions in order to enable this bookkeeping without
	 * any dependencies to the underlying implementation of the publish-subscribe pattern.
	 *
	 * @tparam Connection
	 *    the implementation-specific connection type used by the publisher
	 * @param[in] publisher
	 *    the publisher owning the connection to be closed
	 * @param[in] connection
	 *    the connection to be closed, will be invalid afterwards
	 */
	template< typename Connection >
	static void disconnect( Publisher& publisher, Connection&& connection ) {
		publisher.disconnect( std::forward< Connection >(connection) );
	}

	/**
	 * @brief Register a new subscription at the publisher.
	 *
	 * This bookkeeping method is invoked when a new subscription has been created.
	 * Publishers should maintain a reference to all their subscriptions in order to keep them
	 * alive as long as the publisher instance exists and close them when the latter is destroyed.
	 *
	 * @tparam Subscription
	 *    the actual subscription type
	 * @param[in] publisher
	 *    the publisher where the subscription shall be registered
	 * @param[in] subscription
	 *    the subscription to be registered at the publisher
	 */
	template< typename Subscription >
	static void addSubscription( Publisher& publisher, Subscription& subscription ) {
		publisher.addSubscription( subscription );
	}

	/**
	 * @brief Unregister a subscription from the publisher.
	 *
	 * This bookkeeping method is invoked when a publisher's subscription is closed.
	 * It will be used to update all references to the publisher's subscriptions.
	 *
	 * @tparam Subscription
	 *    the actual subscription type
	 * @param[in] publisher
	 *    the publisher where the subscription shall be unregistered
	 * @param[in] subscription
	 *    the subscription to be unregistered at the publisher
	 */
	template< typename Subscription >
	static void removeSubscription( Publisher& publisher, Subscription& subscription ) {
		publisher.removeSubscription( subscription );
	}
};


#endif /* PUBLISHERTRAITS_HPP_ */
