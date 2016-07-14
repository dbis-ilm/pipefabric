/*
 * ChannelTraits.hpp
 *
 *  Created on: Feb 9, 2015
 *      Author: fbeier
 */

#ifndef CHANNELTRAITS_HPP_
#define CHANNELTRAITS_HPP_

#include <cstddef>


/**
 * @brief Properties a @c Channel implementation must satisfy.
 *
 * @tparam _Channel
 *    the @c Channel implementation class whose traits are defined here
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template< typename _Channel >
struct ChannelTraits {
	//////   public types   //////

	typedef _Channel Channel; /**< the channel implementation type */
	typedef typename Channel::ChannelID ChannelID;       /**< the compile-time channel identifier */
	typedef typename Channel::Component Component;       /**< the component where the channel is bound */
	typedef typename Channel::ChannelTypes ChannelTypes; /**< the channel data types */


	//////   public constants   //////

	/// the compile-time flag indicating if the channel acts as input our output channel
	static const bool IS_INPUT_CHANNEL = _Channel::IS_INPUT_CHANNEL;


	//////   public interface   //////

	/**
	 * @brief Get a reference to the component the channel is bound to.
	 *
	 * @param[in] channel
	 *    the channel instance whose component shall be returned
	 * @return a reference to the component the channel is bound to
	 */
	static Component& getBoundComponent( Channel& channel ) {
		return channel.getBoundComponent();
	}

	/**
	 * @brief Get a const reference to the component the channel is bound to.
	 *
	 * @param[in] channel
	 *    the channel instance whose component shall be returned
	 * @return a const reference to the component the channel is bound to
	 */
	static const Component& getBoundComponent( const Channel& channel ) {
		return channel.getBoundComponent();
	}

	/**
	 * @brief Get the number of subscription that are registered for the channel.
	 *
	 * @param[in] channel
	 *    the channel instance whose number of subscriptions shall be returned
	 * @return the number of registered subscriptions.
	 */
	static std::size_t getNumSubscriptions( const Channel& channel ) {
		return channel.getNumSubscriptions();
	}

protected:

	/**
	 * @brief Register a specific subscription for a channel.
	 *
	 * This method will add a subscription reference to this channel instance.
	 * Only the registry entry is added here.
	 *
	 * @tparam Subscription
	 *     the subscription type
	 * @param[in] channel
	 *    the channel instance where the subscription shall be added
	 * @param[in] subscription
	 *     the subscription instance to be registered
	 */
	template< typename Subscription >
	static void addSubscription( Channel& channel, Subscription& subscription ) {
		channel.addSubscription( subscription );
	}

	/**
	 * @brief Unregister a specific subscription from the channel.
	 *
	 * This method will remove a subscription reference from this channel instance.
	 * Only the registry entry is removed here.
	 *
	 * @tparam Subscription
	 *     the subscription type
	 * @param[in] channel
	 *    the channel instance where the subscription shall be removed
	 * @param[in] subscription
	 *     the subscription instance to be removed
	 */
	template< typename Subscription >
	static void removeSubscription( Channel& channel, Subscription& subscription ) {
		channel.removeSubscription( subscription );
	}
};


#endif /* CHANNELTRAITS_HPP_ */
