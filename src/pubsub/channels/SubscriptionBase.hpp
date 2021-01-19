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
