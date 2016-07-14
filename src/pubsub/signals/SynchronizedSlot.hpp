/*
 * SynchronizedSlot.hpp
 *
 *  Created on: Feb 3, 2015
 *      Author: fbeier
 */

#ifndef SYNCHRONIZEDSLOT_HPP_
#define SYNCHRONIZEDSLOT_HPP_

#include <type_traits>
#include <algorithm>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>


template< class SlotImpl >
class SynchronizedSlot;


namespace impl {

/**
 * @brief A type trait indicating if a type is a @c SynchronizedSlot instantiation.
 *
 * This is the default trait instantiation which resolves to @c false.
 *
 * @tparam SlotImpl
 *    the type to be tested
 */
template< typename SlotImpl >
struct isSynchronizedSlot : std::false_type {};

/**
 * @brief A type trait indicating if a type is a @c SynchronizedSlot instantiation.
 *
 * This is the trait instantiation for a @c SynchronizedSlot which resolves to @c true.
 *
 * @tparam SlotImpl
 *    the wrapped slot type by the @c SynchronizedSlot
 */
template< typename SlotImpl >
struct isSynchronizedSlot< SynchronizedSlot< SlotImpl > > : std::true_type {};

} /* end namespace impl */


/**
 * @brief A synchronized function callback.
 *
 * This class represents a function callback that synchronizes its invocations between
 * multiple threads. It is implemented as a decorator around an underlying slot implementation
 * and uses a mutex to serialize concurrent invocations of the slot's @c operator().
 *
 * @tparam SlotImpl
 *           the underlying slot implementation whose invocations shall be synchronized
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	class SlotImpl
>
class SynchronizedSlot {
private:

	/// the underlying slot implementation
	typedef SlotImpl Slot;

	/// a mutex type used for synchronizing slot invocations
	typedef boost::recursive_mutex SlotMutex;

public:
	template< typename OtherSlot >
	friend class SynchronizedSlot;

	SynchronizedSlot() = default;

	/**
	 * @brief Conversion constructor for a basic slot.
	 *
	 * @tparam OtherSlot
	 *     the underlying slot type to be wrapped in a @c SynchronizedSlot
	 * @param otherSlot
	 *     the slot implementation to be wrapped
	 */
	template<
		typename OtherSlot,
		typename = std::enable_if<
			!impl::isSynchronizedSlot< OtherSlot >::value
		>
	>
	SynchronizedSlot( const OtherSlot& otherSlot ) :
		mSlot( otherSlot ) {
	}

	/**
	 * @brief Conversion copy constructor for a synchronized slot with a different slot implementation.
	 *
	 * @tparam OtherSlot
	 *     the underlying slot implementation of the other @c SynchronizedSlot
	 * @param[in] other
	 *     the @c SynchronizedSlot to be converted
	 */
	template<
		typename OtherSlot
	>
	SynchronizedSlot( const SynchronizedSlot< OtherSlot >& other ) :
		mSlot( other.mSlot ) {
	}


	/**
	 * @brief Conversion copy constructor for a synchronized slot with a different slot implementation.
	 *
	 * @tparam OtherSlot
	 *     the underlying slot implementation of the other @c SynchronizedSlot
	 * @param[in] other
	 *     the @c SynchronizedSlot to be converted
	 */
	SynchronizedSlot( const SynchronizedSlot& other ) :
		mSlot( other.mSlot ) {
	}

	/**
	 * @brief Move constructor.
	 *
	 * @param[in] other
	 *     the other synchronized slot
	 */
	SynchronizedSlot( SynchronizedSlot&& other ) :
		SynchronizedSlot() {
		// copy-and-swap idiom
		swap( *this, other );
	}

	/**
	 * @brief Copy/move assignment operator.
	 *
	 * Taking @c other by value will either copy construct or move construct the
	 * @c SychronizedSlot, depending on the argument type.
	 *
	 * @param[in] other
	 *     the slot to be assigned to this instance
	 *
	 * @return a reference to the updated slot
	 */
	SynchronizedSlot& operator=( SynchronizedSlot other ) {
		// copy-and-swap idiom
		swap( *this, other );
		return *this;
	}

	/**
	 * @brief Swap two slot instances.
	 *
	 * This function will exchange the content of the two slots passed as arguments.
	 * It is required for implementing the copy-and-swap idiom:
	 * http://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	 *
	 * @param[in] first
	 *     the first slot instance
	 * @param[in] second
	 *     the second slot instance
	 */
	friend void swap( SynchronizedSlot& first, SynchronizedSlot& second ) {
		using std::swap;
		swap( first.mSlot, second.mSlot );
		// the mutex cannot be swapped
	}


	/**
	 * @brief Invoke the slot function.
	 *
	 * This function forwards the @c data elements to the underlying slot implementation.
	 * Further, it serializes concurrent invocations of this function.
	 *
	 * @tparam Data
	 *     the types of data elements to be forwarded
	 * @param[in] data
	 *     the data elements forwarded to the underlying slot
	 */
	template< typename... Data >
	void operator() ( Data&&... data ) const {
		boost::lock_guard< SlotMutex > lock( mMutex );
		mSlot( std::forward< Data >( data )... );
	}

private:

	Slot mSlot;               /**< the underlying slot which is decorated here */
	mutable SlotMutex mMutex; /**< the mutex for synchronizing slot invocations */
};


#endif /* SYNCHRONIZEDSLOT_HPP_ */
