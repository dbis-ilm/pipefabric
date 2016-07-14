/*
 * SubscriptionBase.hpp
 *
 *  Created on: Feb 8, 2015
 *      Author: felix
 */

#ifndef SUBSCRIPTIONBASE_HPP_
#define SUBSCRIPTIONBASE_HPP_

#include "libcpp/types/types.hpp"


class SubscriptionBase;

/// a shared pointer to a subscription instance
typedef std::shared_ptr< SubscriptionBase > SubscriptionPtr;


/**
 * @brief Common base class for all subscriptions.
 *
 * This class is required for handling subscriptions in a uniform manner during runtime.
 * It just provides the interface for handling subscriptions of different types
 * (which depends on which publisher and subscriber are used as template arguments).
 * The actual implementation is forwarded to the subscription via runtime polymorphism.
 */
class SubscriptionBase :
	private boost::noncopyable
{
public:

	/**
	 * @brief Virtual destructor.
	 *
	 * Do nothing here, since all method implementations depend on the implementing class
	 * at runtime which is not guaranteed to be existent when the destructor is called.
	 */
	virtual ~SubscriptionBase() {}

	/**
	 * @brief Close the current subscription represented by this handle.
	 */
	void close() {
		closeImpl();
	}

	/**
	 * @brief Check if the current subscription represented by this handle is connected.
	 *
	 * @return @c true if the subscription is connected,
	 *         @c false otherwise
	 */
	bool isConnected() const {
		return isConnectedImpl();
	}

private:

	virtual void closeImpl() = 0;
	virtual bool isConnectedImpl() const = 0;
};


#endif /* SUBSCRIPTIONBASE_HPP_ */
