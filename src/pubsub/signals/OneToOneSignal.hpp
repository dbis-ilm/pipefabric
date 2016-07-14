/*
 * OneToOneSignal.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef ONETOONESIGNAL_HPP_
#define ONETOONESIGNAL_HPP_

#include "SignalTraits.hpp"

#include <type_traits>
#include <cassert>
#include <boost/mpl/assert.hpp>


namespace impl {

/**
 * @brief A signal implementation which can have one subscribing slot at max.
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
class OneToOneSignalImpl {
private:

	/// the internal function callback for publishing data elements
	typedef SlotImpl SlotFunction;
	typedef unsigned int SlotID;
	static const SlotID gSLOT_ID = 0;

public:

	/// the unique ID representing a connection for a specific slot
	typedef SlotID Connection;


	/// this signal implementation is not thread safe
	static const bool IS_THREAD_SAFE = false;

	/// this signal implementation can connect one slot at max
	static const SignalCardinality CARDINALITY = ONE_TO_ONE;


	/**
	 * @brief Connect a new slot callback to the signal.
	 *
	 * This method will fail with a runtime assertion in case the internal slot is
	 * tried to be overridden since only one slot connection is allowed at maximum.
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

		assert( !mSlot );

		mSlot = slot;
		return gSLOT_ID;
	}

	/**
	 * @brief Close the connection for the connected slot.
	 *
	 * @param connection
	 *    the connection to be closed, will be invalid afterwards
	 */
	void disconnect( Connection&& connection ) {
		assert( connection == gSLOT_ID );
		mSlot = SlotFunction();
	}

	/**
	 * @brief Signal invocation for publishing data elements.
	 *
	 * When the signal is invoked, it invokes its internal slot function as delegate
	 * in order to forward the produced data elements to the subscriber.
	 *
	 * @tparam Data
	 *    the types of data published elements, must be convertible to the
	 *    @c PublishedTypes of the signal
	 * @param data
	 *    the data elements which will be forwarded to the slot
	 */
	template< typename... Data >
	void operator() ( Data&&... data ) {
		if( mSlot ) {
			mSlot( std::forward< Data >( data )... );
		}
	}

private:

	SlotFunction mSlot; /**< the internal slot function callback */
};

} /* end namespace impl */


template<
	template< typename... SlotArgs > class SlotImpl,
	typename... PublishedTypes
>
using OneToOneSignal = impl::OneToOneSignalImpl<
	SlotImpl< PublishedTypes... >, PublishedTypes...
>;


#endif /* ONETOONESIGNAL_HPP_ */
