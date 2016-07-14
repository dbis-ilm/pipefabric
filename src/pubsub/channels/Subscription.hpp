/*
 * Subscription.hpp
 *
 *  Created on: Jan 12, 2014
 *      Author: fbeier
 */

#ifndef SUBSCRIPTION_HPP_
#define SUBSCRIPTION_HPP_

#include "SubscriptionBase.hpp"
#include "PublisherTraits.hpp"
#include "SubscriberTraits.hpp"

#include <boost/noncopyable.hpp>


/**
 * @brief Class representing an association between a @c Publisher and a @c Subscriber.
 *
 * This class represents an association between a single data producing component
 * (@c Publisher), e.g., an @c OutputChannel, and a data handling component
 * (@c Subscriber), e.g., an @c InputChannel, implementing a publish-subscribe pattern.
 * It implements automatic connection management, i.e., the RAII pattern around an
 * implementation-specific @c Connection handle, representing the connection between the
 * @c Publisher and the @c Subscriber. This connection is automatically closed
 * when the last reference to this subscription vanished.
 *
 * @tparam Publisher
 *    the data producer type bound to the subscription
 * @tparam Subscriber
 *    the data consumer type bound to the subscription
 * @tparam Connection
 *    the implementation-specific handle representing the connection on publisher side
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename Publisher,
	typename Subscriber,
	typename Connection
>
class Subscription :
	public SubscriptionBase, // runtime polymorphic base
	public ns_types::SharedInstance<
		Subscription< Publisher, Subscriber, Connection >
	>
{
private:

	/// short alias for this type
	typedef Subscription< Publisher, Subscriber, Connection > ThisType;

	/// base type for shared ownership semantics of subscriptions
	typedef ns_types::SharedInstance< ThisType > SharedInstanceBase;

	/// internal key structure for shared instance creation
	typedef typename SharedInstanceBase::CreationKey CreationKey;

	/// interface of the publisher endpoint of the subscription
	typedef PublisherTraits< Publisher > PublisherInterface;

	/// a weak reference to the publisher endpoint to avoid circular dependencies
	typedef typename PublisherInterface::WeakRef WeakPublisherRef;

	/// a shared reference to the publisher endpoint
	typedef typename PublisherInterface::SharedRef PublisherRef;

	/// interface of the subscriber endpoint of the subscription
	typedef SubscriberTraits< Subscriber > SubscriberInterface;

	/// a weak reference to the subscriber endpoint to avoid circular dependencies
	typedef typename SubscriberInterface::WeakRef WeakSubscriberRef;

	/// a shared reference to the subscriber endpoint
	typedef typename SubscriberInterface::SharedRef SubscriberRef;


public:

	/**
	 * @brief Create a new @c Subscription between a @c publisher and a @c subscriber.
	 *
	 * The constructor is hidden to enforce the instance creation through the static
	 * @c create() factory method that returns a unique @c Subscription reference.
	 *
	 * @tparam Publisher
	 *    the publisher implementation type
	 * @tparam Subscriber
	 *    the subscriber implementation type
	 * @tparam Connection
	 *    the implementation-specific connection type between the publisher's signal
	 *    and the subscriber's slot
	 * @param[in] publisher
	 *    the data producer the @c subscriber wants to subscribe
	 * @param[in] subscriber
	 *    the data consumer that subscribes the @c publisher
	 * @param[in] key
	 *    the internal creation key to enforce the shared instance creation via the
	 *    @c create() factory method
	 */
	Subscription( const CreationKey& key, Publisher& publisher, Subscriber& subscriber,
			Connection&& connection ) :
		SharedInstanceBase( key ),
		mPublisher( PublisherInterface::getWeakRef( publisher ) ),
		mSubscriber( SubscriberInterface::getWeakRef( subscriber ) ),
		mConnection( std::move( connection ) ),
		mIsConnected( true ) {
	}

	/**
	 * @brief The subscription destructor.
	 *
	 * The internal connection will be closed when the last reference vanishes
	 * and the subscription it is unregistered from its endpoint.
	 */
	virtual ~Subscription() {
		// no virtual method in destructors
		closeSubscription();
	}


private:

	/**
	 * @brief Close the subscription.
	 *
	 * This method will close the subscription, i.e., its internal connection
	 * is closed and the subscription reference is removed from both, the publisher
	 * and the subscriber endpoint.
	 */
	void closeSubscription() {
		if( mIsConnected == true ) {
			PublisherRef publisher = mPublisher.lock();
			if( publisher ) {
				PublisherInterface::disconnect( *publisher, std::move( mConnection ) );
				PublisherInterface::removeSubscription( *publisher, *this );
			}

			SubscriberRef subscriber = mSubscriber.lock();
			if( subscriber ) {
				SubscriberInterface::removeSubscription( *subscriber, *this );
			}

			mIsConnected = false;
		}
	}

	/**
	 * @brief Runtime polymorphic implementation of closing a subscription
	 *        through the @c SubscriptionBase interface.
	 */
	virtual void closeImpl() override {
		// forward to the non-virtual implementation which is reused in the d'tor
		closeSubscription();
	}

	/**
	 * @brief Check if the subscription is connected
	 *
	 * @return @c true if the internal connection between the @c Producer's and the @c Consumer
	 *         is established,
	 *         @c false otherwise
	 */
	virtual bool isConnectedImpl() const override {
		return mIsConnected;
	}


	WeakPublisherRef mPublisher;   /**< the publisher endpoint */
	WeakSubscriberRef mSubscriber; /**< the subscriber enpoint */
	Connection mConnection;        /**< the wrapped signal slot connection managed through
	                                    the subscription instance */
	bool mIsConnected; /**< flag indicating if the subscription is still connected */
};

#endif /* SUBSCRIPTION_HPP_ */
