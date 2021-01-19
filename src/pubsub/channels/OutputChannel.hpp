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
 * OutputChannel.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: fbeier
 */

#ifndef OUTPUTCHANNEL_HPP_
#define OUTPUTCHANNEL_HPP_

#include "Channel.hpp"
#include "Subscription.hpp"
#include "SubscriberTraits.hpp"
#include "../signals/SignalTraits.hpp"

#include "libcpp/types/types.hpp"

#include <set>


/**
 * @brief A data channel for data producers.
 *
 * This class represents an outgoing data channel for a @c Producer component
 * for a set of data elements of @c ProducedType. Since multiple @c OutputChannels
 * of the same type may exist within the component, the channel is uniquely identified
 * via a @c ChannelID.
 *
 * @c Output channels wrap a @c SignalImpl policy for a publish-subscribe implementation
 * (a.k.a. signal-slot) and satisfy the @c ProducerTraits for providing a common
 * interface for managing subscribers, publishing data elements, and notifying all
 * listening consumers, e.g., @c InputChannels. The common signal interface is provided
 * through the @c SignalTraits class where either the @c SignalImpl type satisfies
 * the default interface, e.g., for custom implementations, or a custom traits
 * specialization is provided for an existing implementation that shall be wrapped.
 * The @c SignalImpl policy provides a way for customizing the actual notification
 * mechanism, i.e.,:
 *   - in which order subscribers are notified
 *   - what call mechanism will be used, e.g., async calls via separate threads,
 *     direct function invocations by a single thread, etc.
 *   - etc.
 *   .
 *
 * There are no requirements regarding the @c Producer endpoint component provided as
 * template argument. It is just required to generate a compile-time unique
 * type for the @c Channel base class. The channel does not access it. The channel
 * merely acts as possible target for produced data elements.
 *
 * There is an 1-N relationship between @c OutputChannels and its subscribers,
 * i.e., a single @c OutputChannel instance can have many listening components
 * if the underlying @c SignalImpl implementation supports this.
 * Regarding the @c Producer component, a N-1 relationship is assumed, i.e., a single
 * @c Producer instance can own many @c OutputChannel instances. But this might be
 * different in the actual @c Producer implementation. The user of this class just
 * needs to make sure to pass a @c ChannelID as parameter which uniquely identifies
 * this channel in order to avoid ambiguities.
 *
 * TODO Is it beneficial to provide a compile-time implementation of publisher/subscriber
 *      chains, possibly with special interfaces that can be manipulated at runtime?
 *      This could be used for a compile-time data flow generation + static optimizations
 *      and e.g., dynamic load monitoring and management during runtime.
 *      Static "monadic" chains could minimize the signal slot overhead through deferred
 *      function invocations through a delegate.
 *
 * @tparam ChannelID
 *     a unique identifier for the channel inside the @c Producer
 * @tparam Producer
 *     the component traits class which produces outgoing data elements
 * @tparam ProducedTypes
 *     a sequence comprising the types of data elements published via this channel
 * @tparam SignalImpl
 *     the underlying signal implementation used to notify connected consumers
 *     about new data elements
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelID,
	typename Producer,
	typename ProducedTypes,
	typename SignalImpl
>
class OutputChannel :
	public Channel< ChannelID, Producer, false, ProducedTypes >,
	public ns_types::SharedInstance<
		OutputChannel< ChannelID, Producer, ProducedTypes, SignalImpl >
	>
{
private:

	/// the base type of this channel implementing subscription management
	typedef Channel< ChannelID, Producer, false, ProducedTypes > ChannelBase;

	/// short alias for this type
	typedef OutputChannel< ChannelID, Producer, ProducedTypes, SignalImpl > ThisType;

	/// shared ownership semantics for channel instances
	typedef ns_types::SharedInstance< ThisType > SharedInstanceBase;

	/// internal key structure for shared instance creation
	typedef typename SharedInstanceBase::CreationKey CreationKey;

	typedef SignalTraits< SignalImpl > SignalInterface;

public:

	/**
	 * @brief Create a new input channel instance.
	 *
	 * The constructor follows the @c SharedInstance factory pattern to enforce shared
	 * instance creation via the static @ create() factory method.
	 *
	 * @param[in] key
	 *    the internal creation key structure to enforce shared instance creation
	 * @param[in] consumer
	 *    the consumer component the new channel instance is bound to
	 */
	OutputChannel( const CreationKey& key, Producer& producer ) :
		ChannelBase( producer ), SharedInstanceBase( key ) {
	}


	/**
	 * @brief Get the producer this channel is bound to.
	 *
	 * @brief a reference to the producer this channel is bound to
	 */
	inline Producer& getProducer() {
		return ChannelBase::getBoundComponent();
	}

	/**
	 * @brief Publish some data elements through this channel instance.
	 *
	 * This method will publish @c data elements to all subscribers that subscribed
	 * to this channel instance via forwarding the call to the signal policy class.
	 *
	 * @tparam PublishedTypes
	 *    the list of data element types to be published
	 * @param[in] data
	 *    the data elements to be published
	 */
	template< typename... PublishedTypes >
	void publish( PublishedTypes&&... data ) {
		// TODO check that the types are convertible to the channel types
		//      for maybe more readable compiler errors instead of
		//      'no known conversion from ... to ...' with long template messages
		SignalInterface::publish( mSignal, std::forward< PublishedTypes >( data )... );
	}

	/**
	 * @brief Establish a connection between this channel instance as publisher and
	 *        a specific subscriber.
	 *
	 * The @c Subscription is created here, getting a reference to both endpoints
	 * and the internal implementation-specific connection handle for closing
	 * the subscriptions later on. As bookkeeping, the subscription is registered
	 * in both endpoints to automatically close it when any of them is destroyed.
	 * The shared handle to the new subscription is also returned which can be used to
	 * manually manage the underlying subscription. But it is not necessary to keep
	 * a reference to it, since the channel's registry will hold another reference
	 * for keeping its subscriptions alive as long as the endpoints exist.
	 *
	 * @tparam Subscriber
	 *    the subscriber type, must satisfy the @c SubscriberTraits
	 * @param[in] subscriber
	 *    the subscriber instance consuming data elements
	 * @return a handle representing the new subscription
	 */
	template< typename Subscriber >
	SubscriptionPtr subscribe( Subscriber& subscriber ) {
		typedef SubscriberTraits< Subscriber > SubscriberInterface;
		typedef typename SubscriberInterface::Slot Slot;
		typedef typename SignalInterface::Connection Connection;
		typedef Subscription< ThisType, Subscriber, Connection > SubscriptionHandle;

		Slot slot = SubscriberInterface::getSlot( subscriber );
		Connection connection = SignalInterface::connect( mSignal, slot );
		// create the subscription, passing ownership of the new connection
		typename SubscriptionHandle::Instance subscription =
			SubscriptionHandle::create( *this, subscriber, std::move( connection ) );

		ChannelBase::addSubscription( *subscription );
		SubscriberInterface::addSubscription( subscriber, *subscription );
		return subscription;
	}


private:

	/// grant access to the publisher interface defined for this channel
	friend class PublisherTraits< ThisType >;

	/**
	 * @brief Close the connection between this publisher and a subscriber instance.
	 *
	 * This method will be called by the @c Subscription through the @c PublisherTraits
	 * interface when it closes itself close the implementation-specific @c Connection
	 * that was created when a subscriber subscribed this channel instance.
	 *
	 * @tparam Connection
	 *    the implementation-specific connection type used by the publisher
	 * @param[in] publisher
	 *    the publisher owning the connection to be closed
	 * @param[in] connection
	 *    the connection to be closed, will be invalid afterwards
	 */
	template< typename Connection >
	void disconnect( Connection&& connection ) {
		SignalInterface::disconnect( mSignal, std::forward< Connection >( connection ) );
	}


	/// the internal signal implementation
	typedef typename SignalInterface::Signal Signal;

	Signal mSignal; /**< the internal signal instance for managing connected subscribers
	                     (a.k.a. slots) and publishing data elements */
};


#endif /* OUTPUTCHANNEL_HPP_ */
