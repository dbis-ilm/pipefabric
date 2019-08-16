/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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
 * SignalTraits.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef SIGNALTRAITS_HPP_
#define SIGNALTRAITS_HPP_

#include <type_traits>


/**
 * @brief Possible cardinalities of signals.
 *
 * The signal cardinality expresses how many slots can be connected to one signal instance at once.
 */
enum SignalCardinality {
	 ONE_TO_ONE  /**< the signal allows one simultaneous slot connection at max */
	,ONE_TO_MANY /**< the signal allows many simultaneous slot connections at max */
};


/**
 * @brief Traits class for signal implementations.
 *
 * This class defines the public properties and interface that must be provided for arbitrary
 * signal implementations in order to be used by the publish subscribe implementation.
 *
 * Specialize the class for all signal implementations that do not implement the
 * default interface.
 *
 * @tparam SignalImpl
 *           the signal implementation for which the traits are defined
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename SignalImpl
>
class SignalTraits {
public:
	//////   public types   //////

	/// the internal signal implementation
	typedef SignalImpl Signal;
	static_assert( std::is_default_constructible< Signal >::value == true,
		"a signal implementation must be default constructible"
	);

	/// an implementation-specific handle for a single signal-slot connection
	typedef typename Signal::Connection Connection;
	static_assert( std::is_copy_constructible< Connection >::value == true,
		"a signal-slot connection must be copy constructible"
	);


	//////   public constants   //////

	/// flag indicating if the underlying signal implementation is tread-safe
	static const bool IS_THREAD_SAFE = Signal::IS_THREAD_SAFE;

	/// the cardinality of the underlying signal
	static const SignalCardinality CARDINALITY = Signal::CARDINALITY;


	//////   public interface   //////

	/**
	 * @brief Establish a connection between the signal and a specific slot.
	 *
	 * This method registers the @c slot for the data that is published via the @c signal
	 * passed as argument. The return value represents an implementation-specific handle
	 * for the new @c Connection between them. It can be used to close the connection
	 * via the @c disconnect() method.
	 *
	 * @tparam Slot
	 *    the callback type to be connected as slot to the signal
	 * @param[in] signal
	 *    the signal which shall be connected
	 * @param[in] slot
	 *    the slot to be connected to the signal
	 * @return a unique handle representing the new connection between this signal and the slot
	 */
	template< typename Slot >
	static Connection connect( Signal& signal, const Slot& slot ) {
		return signal.connect( slot );
	}

	/**
	 * @brief Disconnect a specific slot from the signal.
	 *
	 * This method closes the @c connection that was created via connect for a specific slot
	 * for a specific @c signal. The disconnection operation is implementation-specific.
	 * After disconnecting, the @c connection handle will be regarded invalid.
	 * Therefore, passing it as rvalue reference will enforce this.
	 *
	 * @param[in] signal
	 *    the signal whose connection shall be close
	 * @param[in] connection
	 *    the connection which shall be closed, will be invalid afterwards
	 */
	static void disconnect( Signal& signal, Connection&& connection ) {
		signal.disconnect( std::move( connection ) );
	}


	/**
	 * @brief Publish some data through the signal.
	 *
	 * This method will invoke the implementation-specific call mechanism that is used
	 * by the @c signal to notify its registered slots that new @c data is available.
	 * The @c data elements are perfectly forwarded to the @c signal.
	 *
	 * @tparam[PublishedDataTypes]
	 *    the types of the data elements to be published
	 * @param[in] signal
	 *    the signal implementation which shall publish the data
	 * @param[in] data
	 *    a list of data elements to be published
	 */
	template< typename... PublishedDataTypes >
	static void publish( Signal& signal, PublishedDataTypes&&... data ) {
		signal( std::forward< PublishedDataTypes >( data )... );
	}
};


#endif /* SIGNALTRAITS_HPP_ */
