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
 * InputChannel.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: fbeier
 */

#ifndef INPUTCHANNEL_HPP_
#define INPUTCHANNEL_HPP_

#include "Channel.hpp"
#include "impl/ChannelConsumer.hpp"
#include "PublisherTraits.hpp"

#include "libcpp/types/types.hpp"


/**
 * @brief A data channel for data consumers.
 *
 * This class represents an incoming data channel for a @c Consumer component
 * for a set of data elements of @c ConsumedTypes. Since multiple @c InputChannels
 * of the same type may exist within the component, the channel is uniquely identified
 * via a @c ChannelID.
 *
 * The binding to the @c SlotImpl callback of the underlying @c Consumer component
 * that shall be used to handle incoming data elements is done via the @c getSlot() method.
 * Therefore, the @c Consumer must be a @c ChannelConsumer for this channel type and provide a
 * callback for this channel type which will be called when new data elements arrive.
 * The @c SlotImpl parameter is passed as policy class in order to customize callback
 * behavior, e.g.:
 *   - a simple function object (e.g., @c BoostSlot, @c StdSlot)
 *   - a decorated function object for synchronizing concurrent invocations of the slot
 *     (e.g., @c SynchronizedSlot)
 *   .
 *
 * @c InputChannels satisfy the @c SubscriberTraits and can be bound to classes satisfying
 * the @c PublisherTraits, e.g., @c OutputChannels. Therefore, a @c Subscription is created
 * for receiving data elements produced by publishers having the same (or convertible) types
 * like the @c InputChannel consumes (@c ConsumedTypes). Channels are responsible for
 * managing its @c Subscriptions which are automatically closed when any of its endpoints
 * is destroyed.
 *
 * An @c InputChannel implements a 1-N relationship to publishers, i.e., a single
 * @c InputChannel instance can have many connected data sources represented through
 * separate @c Subscriptions each.
 * Regarding the @c Consumer component, a N-1 relationship is assumed, i.e., a single
 * @c Consumer instance can own many @c InputChannel instances. But this might be
 * different in the actual @c Consumer implementation. The user of this class just
 * needs to make sure to pass a @c ChannelID as parameter which uniquely identifies
 * this channel in order to avoid ambiguities. To which @c Consumer slot the channel is
 * bound to is determined at runtime when a new @c Subscription is created via @c subscribe().
 *
 * TODO Is it beneficial to provide a compile-time implementation of publisher/subscriber
 *      chains, possibly with special interfaces that can be manipulated at runtime?
 *      This could be used for a compile-time data flow generation + static optimizations
 *      and e.g., dynamic load monitoring and management during runtime.
 *      Static "monadic" chains could minimize the signal slot overhead through deferred
 *      function invocations through a delegate.
 *
 * @tparam ChannelID
 *     a unique identifier for the channel inside the @c Consumer
 * @tparam Consumer
 *     the consumer component which handles incoming data elements
 * @tparam ConsumedTypes
 *     a sequence comprising the types of incoming data elements through this channel
 * @tparam SlotImpl
 *     the underlying slot implementation used as callback to notify the @c Consumer
 *     about incoming data elements
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename ChannelID,
	typename Consumer,
	typename ConsumedTypes,
	typename SlotImpl
>
class InputChannel :
	public Channel< ChannelID, Consumer, true, ConsumedTypes >,
	public ns_types::SharedInstance<
		InputChannel< ChannelID, Consumer, ConsumedTypes, SlotImpl >
	>
{
private:

	/// the base type of this channel implementing subscription management
	typedef Channel< ChannelID, Consumer, true, ConsumedTypes > ChannelBase;

	/// short alias for this type
	typedef InputChannel< ChannelID, Consumer, ConsumedTypes, SlotImpl > ThisType;

	/// shared ownership semantics for channel instances
	typedef ns_types::SharedInstance< ThisType > SharedInstanceBase;

	/// internal key structure for shared instance creation
	typedef typename SharedInstanceBase::CreationKey CreationKey;


public:

	/// the slot type of the consumer component this channel is bound to
	typedef SlotImpl Slot;

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
	InputChannel( const CreationKey& key, Consumer& consumer ) :
		ChannelBase( consumer ), SharedInstanceBase( key ) {
	}


	/**
	 * @brief Establish a connection between this channel instance as subscriber and
	 *        a specific publisher.
	 *
	 * The @c Subscription is created by the publisher itself since it provides the
	 * actual implementation for it. It will also handle necessary bookkeeping
	 * operations for managing the subscriptions in both endpoints.
	 * The shared handle to the new subscription is returned which can be used to
	 * manually manage the underlying subscription. But it is not necessary to keep
	 * a reference to it, since the channel's registry will hold another reference
	 * for keeping its subscriptions alive as long as the endpoints exist.
	 *
	 * @tparam Publisher
	 *    the publisher type, must satisfy the @c PublisherTraits
	 * @param[in] publisher
	 *    the publisher instance producing data elements
	 * @return a handle representing the new subscription
	 */
	template< typename Publisher >
	SubscriptionPtr subscribe( Publisher& publisher ) {
		typedef PublisherTraits< Publisher > PublisherInterface;
		return PublisherInterface::subscribe( publisher, *this );
	}

	/**
	 * @brief Get the data processing callback used for this channel instance.
	 *
	 * This method gets a callback method that is used by the underlying consumer
	 * endpoint to handle incoming data elements by this channel.
	 *
	 * @return the underlying slot for this channel
	 */
	Slot getSlot() {
		typedef impl::ChannelConsumer< InputChannel > DataHandler;
		auto& dataHandler = static_cast< DataHandler& >( getConsumer() );
		return dataHandler.bindInputChannel( *this );
	}

	/**
	 * @brief Get the consumer this channel is bound to.
	 *
	 * @brief a reference to the consumer this channel is bound to
	 */
	inline Consumer& getConsumer() {
		return ChannelBase::getBoundComponent();
	}
};


#endif /* INPUTCHANNEL_HPP_ */
