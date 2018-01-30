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
 * OneToManySignal.hpp
 *
 *  Created on: Jan 29, 2015
 *      Author: fbeier
 */

#ifndef ONETOMANYSIGNAL_HPP_
#define ONETOMANYSIGNAL_HPP_

#include "SignalTraits.hpp"

#include <boost/foreach.hpp>
#include <boost/mpl/assert.hpp>
#include <vector>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <cassert>

namespace impl {

/**
 * @brief A signal implementation which can have multiple subscribing slots.
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
class OneToManySignalImpl {
private:

	/// the internal function callback for publishing data elements
	typedef SlotImpl SlotFunction;

	/// a unique ID for identifying each slot connection
	typedef unsigned int SlotID;

	/// an entry for a registered slot
	typedef std::pair< SlotID, SlotFunction > SlotEntry;


public:

	/// the unique ID representing a connection for a specific slot
	typedef SlotID Connection;


	/// this signal implementation is not thread safe
	static const bool IS_THREAD_SAFE = false;

	/// this signal implementation can connect many slots
	static const SignalCardinality CARDINALITY = ONE_TO_MANY;


	/**
	 * @brief Create a new one-to-many signal instance.
	 */
	OneToManySignalImpl() :
		mNextSlotID( 0 ) {
	}


	/**
	 * @brief Connect a new slot callback to the signal.
	 *
	 * @tparam Slot
	 *    the type of the callback to be connected as slot,
	 *    must be convertible to the internal @c SlotImpl type specified as class template argument
	 * @param slot
	 *    the callback to be connected as slot
	 * @return a unique handle representing the new connection between this signal and the slot
	 */
	template< typename Slot >
	Connection connect( const Slot& slot ) {
		BOOST_MPL_ASSERT_MSG(
			(std::is_convertible< Slot, SlotFunction >::value),
			UNCOMPATIBLE_SUBSCRIBING_SLOT,
			(Slot, SlotFunction)
		);

		SlotID slotID = mNextSlotID++; // TODO this could overflow
		mSlots.push_back( std::make_pair( slotID, slot ) );
		return slotID;
	}

	/**
	 * @brief Close a connection for a specific slot.
	 *
	 * @param connection
	 *    the connection to be closed, will be invalid afterwards
	 */
	void disconnect( Connection&& connection ) {
		auto entry = std::find_if( mSlots.begin(), mSlots.end(),
			[&connection]( const SlotEntry& entry ) -> bool {
				return entry.first == connection;
			}
		);

		assert( entry != mSlots.end() );
		mSlots.erase( entry );
	}


	/**
	 * @brief Signal invocation for publishing data elements.
	 *
	 * When the signal is invoked, it invokes its internal slot functions as delegate
	 * in order to forward the produced data elements to the subscribers.
	 * Slots are invoked in the order they were connected to the signal.
	 *
	 * @tparam Data
	 *    the types of data published elements, must be convertible to the
	 *    @c PublishedTypes of the signal
	 * @param data
	 *    the data elements which will be forwarded to the slot
	 */
	template< typename... Data >
	void operator() ( Data&&... data ) {
		BOOST_FOREACH( const SlotEntry& slotEntry, mSlots ) {
			const SlotFunction& slot = slotEntry.second;
			if( slot ) {
				slot( std::forward< Data >( data )... );
			}
		}
	}


private:

	/// a list for storing all subscribing slots
	typedef std::vector< SlotEntry > Slots;

	SlotID mNextSlotID; /**< the ID to be used for the next slot that connects */
	Slots mSlots;       /**< the internal slot function callbacks */
};

} /* end namespace impl */

template<
	template< typename... SlotArgs > class SlotImpl,
	typename... PublishedTypes
>
using OneToManySignal = impl::OneToManySignalImpl<
	SlotImpl< PublishedTypes... >, PublishedTypes...
>;


#endif /* ONETOMANYSIGNAL_HPP_ */
