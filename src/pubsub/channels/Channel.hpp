/*
 * Channel.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: fbeier
 */

#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include "ChannelID.hpp"
#include "ChannelTraits.hpp"
#include "SubscriptionBase.hpp"

#include <cassert>
#include <type_traits>
#include <set>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/is_sequence.hpp>


/**
 * @brief A single independent data channel of a component of a specific type.
 *
 * A channel represents a single unidirectional flow of information of a specific
 * set of data types @c Types from or to a component. It will be used to identify
 * which data producer is registered for which handler implemented by data consumers.
 *
 * The actual types of data passed between producing and consuming components is
 * specified via the @c Types template argument which must be a compile-time type sequence.
 * Since multiple channels of the same types may exist for a single component, a constant
 * identifier @c ID needs generated during compile time to be able to uniquely identify
 * each of them.
 *
 * Each @c Channel is bound to a @c Comp component as endpoint via reference, i.e.,
 * the endpoint owns its channels and it must make sure that the reference never
 * becomes invalid as long as its channel instances exist and are used.
 *
 * The @c Channel base class provides a common interface for managing @c Subscription,
 * i.e., associations between different @c Channel instances for sending and
 * receiving data elements to/from its endpoints. @c Subscriptions are created
 * in implementing @c InputChannel and @c OutputChannel subclasses and use the
 * functionality of this base class for bookkeeping and automatically close
 * all managed subscriptions when the channel instance is destroyed.
 *
 * @tparam ID
 *     unique compile-time identifier for the channel inside the component
 * @tparam Comp
 *     the component the channel is bound to
 * @tparam IsInputChannel
 *     flag indicating if the channel is acting as an input channel for the
 *     component @c Comp (if @c true) or as output channel (if @c false)
 * @tparam Types
 *     the sequence comprising the channel data types
 */
template<
	typename ID,
	typename Comp,
	bool IsInputChannel,
	typename Types
>
class Channel {
	BOOST_MPL_ASSERT(( std::is_same< typename ID::value_type, ChannelIDValue > ));
	BOOST_MPL_ASSERT(( boost::mpl::is_sequence< Types > ));

public:

	typedef ID ChannelID;       /**< the compile-time channel identifier */
	typedef Comp Component;     /**< the component where the channel is bound */
	typedef Types ChannelTypes; /**< the channel data types */

	/// the compile-time flag indicating if the channel acts as input our output channel
	static const bool IS_INPUT_CHANNEL = IsInputChannel;


	/**
	 * @brief Create a new channel instance bound to a specific component.
	 *
	 * @param[in] comp
	 *    the endpoint component the new channel instance is bound to
	 */
	Channel( Comp& comp ) :
		mComp( comp ) {
	}

	/**
	 * @brief Channel destructor.
	 *
	 * When the channel instance is destroyed, all its subscriptions will be closed.
	 */
	~Channel() {
		while( !mSubscriptions.empty() ) {
			SubscriptionPtr subscription = *(mSubscriptions.begin());
			mSubscriptions.erase( mSubscriptions.begin() ); // will invalidate begin() iter

			assert( subscription );
			subscription->close();
		}
	}


	/**
	 * @brief Get a reference to the component the channel is bound to.
	 *
	 * @return a reference to the component the channel is bound to
	 */
	inline Comp& getBoundComponent() {
		return mComp;
	}

	/**
	 * @brief Get a const reference to the component the channel is bound to.
	 *
	 * @return a const reference to the component the channel is bound to
	 */
	inline const Comp& getBoundComponent() const {
		return mComp;
	}

	/**
	 * @brief Get the number of subscription that are registered for the channel.
	 *
	 * @return the number of registered subscriptions.
	 */
	std::size_t getNumSubscriptions() const {
		return mSubscriptions.size();
	}


	/**
	 * @brief Register a specific subscription for the channel.
	 *
	 * This method will add a subscription reference to this channel instance.
	 * Only the registry entry is added here.
	 *
	 * @tparam Subscription
	 *     the subscription type
	 * @param[in] subscription
	 *     the subscription instance to be registered
	 */
	template< typename Subscription >
	void addSubscription( Subscription& subscription ) {
		SubscriptionPtr subscriptionPtr = subscription.getSharedRef();
		assert( subscriptionPtr );
		mSubscriptions.insert( subscriptionPtr );
	}

	/**
	 * @brief Unregister a specific subscription from the channel.
	 *
	 * This method will remove a subscription reference from this channel instance.
	 * Only the registry entry is removed here.
	 *
	 * @tparam Subscription
	 *     the subscription type
	 * @param[in] subscription
	 *     the subscription instance to be removed
	 */
	template< typename Subscription >
	void removeSubscription( Subscription& subscription ) {
		SubscriptionPtr subscriptionPtr = subscription.getSharedRef();
		assert( subscriptionPtr );
		mSubscriptions.erase( subscriptionPtr );
	}


private:

	/// a set for storing all subscriptions for data published via this channel
	typedef std::set< SubscriptionPtr > Subscriptions;


	Comp& mComp; /**< reference to the component the channel is bound to at runtime */
	Subscriptions mSubscriptions; /**< all subscriptions for this channel */
};


#endif /* CHANNEL_HPP_ */
