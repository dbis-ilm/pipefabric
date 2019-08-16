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
 * BoostSignal.hpp
 *
 *  Created on: Jan 8, 2015
 *      Author: fbeier
 */

#ifndef BOOSTSIGNAL_HPP_
#define BOOSTSIGNAL_HPP_

#include "SignalTraits.hpp"

#include <boost/signals2.hpp>
#include <boost/core/ignore_unused.hpp>
#include <type_traits>

namespace impl {

/**
 * @brief A thread-safe signal implementation using the @c boost::signals2::signal.
 *
 * @note Using a template alias definition would be great, but unfortunately cannot be
 *       specialized below when explicilty specializing the traits class for the signal.
 *       Therefore, we use inheritance here which allows specializations.
 * @see http://stackoverflow.com/questions/17815276/best-way-or-workaround-to-specialize-a-template-alias
 *
 * @tparam SlotImpl
 *    the internal function callback implementation used as slot
 * @tparam PublishedTypes
 *    a list of types published through this signal
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename SlotImpl,
	typename... PublishedTypes
>
struct BoostSignalImpl :
	public boost::signals2::signal<
		void ( PublishedTypes... ),                 // signal signature
		boost::signals2::optional_last_value<void>, // default parameter for combiner
		int, std::less<int>,                        // default parameter for signal groups
		SlotImpl                                    // the underlying slot used as delegate
	>
{};

} /* end namespace impl */

template<
	template< typename... SlotArgs > class SlotImpl,
	typename... PublishedTypes
>
using BoostSignal = impl::BoostSignalImpl<
	SlotImpl< PublishedTypes... >, PublishedTypes...
>;


/**
 * @brief SignalTraits class specialization for the @c BoostSignal.
 *
 * This traits class specialization overrides the default behavior for signals for
 * working with a @c BoostSignal implementation through the common interface.
 *
 * @tparam SlotImpl
 *    the internal function callback implementation used as slot
 * @tparam PublishedTypes
 *    a list of types published through this signal
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename SlotImpl,
	typename... PublishedTypes
>
class SignalTraits<
	impl::BoostSignalImpl< SlotImpl, PublishedTypes... >
>
{
public:
	//////   public types   //////

	/// the internal signal implementation
	typedef impl::BoostSignalImpl< SlotImpl, PublishedTypes... > Signal;

	static_assert( std::is_default_constructible< Signal >::value == true,
		"a signal implementation must be default constructible"
	);

	/// an implementation-specific handle for a single signal-slot connection
	typedef  boost::signals2::connection Connection;
	static_assert( std::is_copy_constructible< Connection >::value == true,
		"a signal-slot connection must be copy constructible"
	);


	//////   public constants   //////

	/// the underlying boost signal implementation is tread-safe
	static const bool IS_THREAD_SAFE = true;

	/// the underlying boost signal implementation allows many connections
	static const SignalCardinality CARDINALITY = ONE_TO_MANY;


	//////   public interface   //////

	/**
	 * @brief Establish a connection between the signal and a specific slot.
	 *
	 * This method registers the @c slot for the data that is published via the @c signal
	 * passed as argument. The return value represents an implementation-specific handle
	 * for the new @c Connection between them. It can be used to close the connection
	 * via the @c disconnect() method.
	 *
	 * The new slot is connected at the back of the underlying signal to guarantee the
	 * invocation order as they were connected.
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
		return signal.connect( slot, boost::signals2::at_back );
	}

	/**
	 * @brief Disconnect a specific slot from the signal.
	 *
	 * This method closes the @c connection that was created via connect for a specific slot
	 * for a specific @c signal. The disconnection operation is implementation-specific.
	 * After disconnecting, the @c connection handle will be regarded invalid.
	 * Therefore, passing it as rvalue reference will enforce this.
	 *
	 * Since the @c BoostSignal already returns a connection handle that can close the
	 * connection itself, nothing special needs to be done here, just disconnect is
	 * invoked on it.
	 *
	 * @param[in] signal
	 *    the signal whose connection shall be close
	 * @param[in] connection
	 *    the connection which shall be closed, will be invalid afterwards
	 */
	static void disconnect( Signal& signal, Connection&& connection ) {
		boost::ignore_unused( signal );
		connection.disconnect();
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


#endif /* BOOSTSIGNAL_HPP_ */
