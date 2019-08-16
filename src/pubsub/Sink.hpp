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
 * Sink.hpp
 *
 *  Created on: Jan 29, 2015
 *      Author: fbeier
 */

#ifndef SINK_HPP_
#define SINK_HPP_

#include "signals/DefaultSlotFunction.hpp"
#include "channels/parameters/InputChannelParameters.hpp"
#include "channels/impl/GenerateInputChannelGroup.hpp"
#include "channels/impl/GenerateChannelConsumerInterface.hpp"
#include "channels/impl/CreateChannelInstanceTypes.hpp"
#include "channels/impl/ChannelCreator.hpp"

#include <cassert> // remove when exception was implemented
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/size.hpp>


// some convenient short cuts
#include "channels/ConnectChannels.hpp"
#include "SinkHelperMacros.hpp"


namespace channels {

	/**
	 * @brief A type representing a parameter for creating an unsynchronized input channel
	 *        using the @c DefaultSlotFunction implementation as delegate.
	 *
	 * @tparam ConsumedTypes
	 *            the list with data types which are received through the input channel
	 *
	 * @author Felix Beier <felix.beier@tu-ilmenau.de>
	 */
	template<
		typename... ConsumedTypes
	> struct In :
		public InputChannelParameters<
			false,
			DefaultSlotFunction,
			ConsumedTypes...
		>
	{};

	/**
	 * @brief A type representing a parameter for creating a synchronized input channel
	 *        using the @c DefaultSlotFunction implementation as delegate.
	 *
	 * @tparam ConsumedTypes
	 *            the list with data types which are received through the input channel
	 *
	 * @author Felix Beier <felix.beier@tu-ilmenau.de>
	 */
	template<
		typename... ConsumedTypes
	> struct SyncIn :
		public InputChannelParameters<
			true,
			DefaultSlotFunction,
			ConsumedTypes...
		>
	{};

} /* end namespace channels */


namespace impl {

/**
 * @brief An interface for consuming data elements.
 *
 * A @c Sink provides the interface for a subscriber in a publish-subscribe pattern.
 * It wraps a group of @c InputChannels for receiving data, one for each independent
 * set of data element types that shall be consumed. The channel implementation to
 * be created is described by a set of @c InputChannelParameters which is passed as
 * template argument list.
 *
 * To receive data elements, the @c Sink's @c InputChannels must be connected to
 * @c OutputChannels of matching type (same or convertible list of types in the signature)
 * which act as publishers and are provided by @c Source components.
 * Two channels can be connected via the @c connectChannels() helper method where
 * each @c InputChannel can have multiple incoming connections, either from the same or
 * from different @c OutputChannels as long as their signatures match.
 *
 * Each @c InputChannel can be separately accessed with the @c getInputChannelByID()
 * method, its type via the nested @c getInputChannelTypeByID meta function.
 * Both require a @c ChannelID as static argument. This ID is generated from the order
 * in which the respective channel was declared. To simplify the usage, some helper
 * macros and structures exist for generating necessary boilerplate code.
 *
 * For each @c InputChannel, the @c Sink must provide at least one callback that is
 * invoked when new data elements arrive through the channel (metaphorically speaking
 * "through the channel" - the callbacks are invoked directly by the publisher without
 * another level of indirection through the channel). These "slot" callbacks, are
 * bound at runtime via the
 * @code
 *     virtual typename <InputChannelType>::Slot bindInputChannel( <InputChannelType>& ) override
 * @endcode
 * method that must be implemented by each @c Sink subclass. This method will be invoked
 * when a specific @c InputChannel of the @c Sink is connected to an @c OutputChannel.
 * The result slot can be bound dynamically, i.e., it is neither necessary that the same
 * @c InputChannel does always bind to the same callback, nor that two @c InputChannels
 * with matching signatures have to bind to different callbacks. For the default case where
 * always the same member function with a matching signature is used as callback,
 * the @c BIND_INPUT_CHANNEL_DEFAULT macro can be used to generate the boilerplate code.
 *
 * @note The @c Sink's @c InputChannels are considered to model independent incoming
 *       data flows of specific sets of elements where each set is treated as atomic unit
 *       like a tuple. This does NOT model the control flow, i.e., how, in which order,
 *       or by which thread the channel's callbacks are invoked. This depends on the actual
 *       implementation used at the connected publisher endpoint.
 *       If the @c Sink implementation and the application has some requirements regarding
 *       these aspects, it must implement the logic itself, i.e., protect shared state with
 *       mutexes in multi-threaded applications etc.
 *       If a single @c InputChannel instance connects to multiple @c OutputChannels
 *       that publish in independent threads, an intra-channel synchronization can be
 *       injected using a @c SynchronizedSlot which acts as decorator for a basic slot
 *       implementation. It will serialize concurrent invocations of the underlying
 *       callback but does not concurrent callback invocations of independent channels.
 *
 * Example:
 *
 * @code
 *
 *     // declare a specific type for a sink with three independent incoming channels
 *     // ID 0 - unsynchronized channel with default slot and types: int, double
 *     // ID 1 - synchronized channel with default slot and types: const std::string&
 *     // ID 2 - manually declared unsynchronized channel with StdSlot and types: int*, char
 *     class SampleSink :
 *         public Sink<
 *               // utility generator for input channel parameters
 *               channels::In< int, double >            // unsynchronized version
 *             , channels::SyncIn< const std::string& > // synchronized version
 *               // explicit channel parameter type specialization
 *             , InputChannelParameters< false, StdSlot, int*, char >
 *         >
 *     {
 *         typedef Sink<
 *               channels::In< int, double >
 *             , channels::SyncIn< const std::string& >
 *             , InputChannelParameters< false, Std::Slot, int*, char >
 *         > Base;
 *
 *
 *     public:
 *
 *         /////   get more readable names for the channels
 *
 *         // directly invoking the channel type meta function
 *         typedef typename Base::template getInputChannelTypeByID< 0 >::type FirstInputChannel;
 *
 *         // using helper macro
 *         IMPORT_INPUT_CHANNEL_TYPE( Base, 1, SecondInputChannel );
 *         IMPORT_INPUT_CHANNEL_TYPE( Base, 2, ThirdInputChannel );
 *
 *
 *         /////   ...
 *
 *         SampleSink( const std::string& name ) : Base( name ) {}
 *
 *
 *         /////   provide runtime-polymorphic channel binding callbacks
 *         //      (note: the methods are dispatched unambiguously via channel reference parameter)
 *
 *         // via explicit method overriding for non-default behavior (see BIND_INPUT_CHANNEL_DEFAULT macro)
 *         virtual typename FirstInputChannel::Slot bindInputChannel( const FirstInputChannel& c ) override {
 *             return std::bind( &SampleSink::processFirstChannel, this,
 *		           std::ref( c ), // inject additional argument
 *	               std::placeholders::_1,
 *	               std::placeholders::_2,
 *	               std::placeholders::_3
 *	           );
 *         }
 *
 *         // via helper macro for default binding to member functions having a matching signature
 *         BIND_INPUT_CHANNEL_DEFAULT( SecondInputChannel, SampleSink, processSecondChannel );
 *         BIND_INPUT_CHANNEL_DEFAULT( ThirdInputChannel, SampleSink, processThirdChannel );
 *
 *
 *         /////   provide channel processing interface which will be bound above
 *
 *         void processFirstChannel( const FirstInputChannel& c, int i, double d ) {
 *            // the actual process implementation for each incoming element via the first channel...
 *            std::cout << "SampleSink '" << getName() << "' processing "
 *                << "'" << i << "', '" << d << "' from first channel" << std::endl;
 *         }
 *
 *         // indirectly matching signature of the second channel
 *         // (const std::string& is convertible from channel to std::string by value)
 *         void processSecondChannel( std::string s ) {
 *            // the actual process implementation for each incoming element via the second channel...
 *            std::cout << "SampleSink '" << getName() << "' processing "
 *                << "'" << s << "' from second channel" << std::endl;
 *         }
 *
 *         // directly matching signature of the third channel
 *         // (same list of types)
 *         void processThirdChannel( int* i, char c ) {
 *            // the actual process implementation for each incoming element via the third channel...
 *            std::cout << "SampleSink '" << getName() << "' processing "
 *                << "'" << i << "', '" << c << "' from third channel" << std::endl;
 *         }
 *     };
 *
 *     // ...
 *
 *     /// manually defined signal
 *     template< typename... Args >
 *     using TestSignal = BoostSignal< StdSlot, Args... >;
 *
 *     void testSinkSample() {
 *         SampleSink sink1( "sink1" ); // create a sink instance (this generates the channels)
 *         SampleSink sink2( "sink2" ); // create another sink instance (this generates the channels)
 *
 *         // declare a type for a source with three independent outgoing channels
 *         // with matching signatures to those of SampleSink
 *         typedef Source<
 *               // utility generator for output channel parameters
 *               channels::Out< std::string > // compatible signature to input channel 1
 *             , channels::Out< int, double > // same signature as input channel 0
 *               // explicit channel parameter type specialization
 *             , OutputChannelParameters< TestSignal, int*, char >
 *         > SampleSource;
 *         SampleSource src;
 *
 *         // get references to the channels
 *         typename SampleSink::getInputChannelTypeByID<0>::type& sink1_In0 = sink1.getInputChannelByID<0>();
 *         typename SampleSink::SecondOutputChannel& sink1_In1 = sink1.getInputChannelByID<1>();
 *         auto& sink1_In2 = sink1.getInputChannelByID<2>();
 *
 *         // connect some channels (only In/Out, Out/In parameters compile)
 *         connectChannels( sink1_In0, src.getOutputChannelByID<1>() );
 *         connectChannels( src.getOutputChannelByID<0>(), sink1_In1 );
 *         connectChannels( src.getOutputChannelByID<2>(), sink2.getInputChannelByID<2>() );
 *
 *         // publish some data directly
 *         src.getOutputChannelByID<0>().publish( "Hello World!" );
 *         int i = 2;
 *         src.getOutputChannelByID<1>().publish( i, static_cast<double>(i) );
 *
 *         // publish some data via output channel reference
 *         auto& outChannel = src.getOutputChannelByID<2>();
 *         outChannel.publish( &i, 'x' );
 *     }
 *
 * @endcode
 *
 * @tparam InputChannelParameters
 *    a list with all parameters describing the input channels that shall be created for the sink
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename InputChannelParameters
>
class SinkImpl :
	// generate an consumer interface for each input channel
	public impl::generateChannelConsumerInterface<
		typename impl::generateInputChannelGroup<
			SinkImpl< InputChannelParameters >, InputChannelParameters
		>::type
	>,
	private boost::noncopyable
{
private:

	/// a group of all @c InputChannel types for this sink
	typedef typename impl::generateInputChannelGroup<
		SinkImpl, InputChannelParameters
	>::type InputChannelGroup;

	/// a list with all input channel types
	typedef typename channel_group::getChannels< InputChannelGroup >::type InputChannelTypes;

	/// a fusion map for storing an @c InputChannel instance for each channel type
	typedef typename impl::CreateChannelInstanceTypes< InputChannelTypes >::type InputChannels;


public:

	/// the number of incoming data channels for this sink
	static const size_t NUM_INPUT_CHANNELS = boost::mpl::size< InputChannelParameters >::value;

	BOOST_STATIC_ASSERT_MSG( boost::mpl::size< InputChannelTypes >::value == NUM_INPUT_CHANNELS,
			"unexpected number of generated input channel types" );
	BOOST_STATIC_ASSERT_MSG( boost::fusion::result_of::size< InputChannels >::value == NUM_INPUT_CHANNELS,
			"unexpected number of generated input channel instances" );


	/**
	 * @brief Meta function returning the type of a specific @c InputChannel of the sink.
	 *
	 * @tparam ID
	 *    a number representing the unique channel ID in [0 ... NUM_INPUT_CHANNELS)
	 */
	template< ChannelIDValue ID >
	struct getInputChannelTypeByID {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_INPUT_CHANNELS, "illegal channel ID" );
		typedef typename channel_group::getChannel<
			InputChannelGroup, ChannelID< ID >
		>::type type;
	};


	/**
	 * @brief Create an new sink instance.
	 *
	 * @param[in] name
	 *    optional name assigned to the sink, default empty
	 */
	SinkImpl( std::string name = "" ) :
		mName( std::move(name) ) {

		// TODO remove
		// std::cout << "creating input channels for sink '" << mName << "':" << std::endl;

		// apply the creator for each map entry, it updates the mapped runtime
		// channel instance for each type with a newly created instance
		boost::fusion::for_each( mInputChannels, impl::ChannelCreator< SinkImpl >( *this ) );
	}

	/**
	 * @brief Get the name assigned to the sink.
	 *
	 * @return the sink's name
	 */
	const std::string& getName() const {
		return mName;
	}

	/**
	 * @brief Get a reference to a specific @c InputChannel.
	 *
	 * @tparam ID
	 *    a number representing the unique channel ID in [0 ... NUM_INPUT_CHANNELS)
	 * @return a reference to the requested channel
	 */
	template< ChannelIDValue ID >
	typename getInputChannelTypeByID< ID >::type&
	getInputChannelByID() const {
		// determine the type of the requested channel as key
		typedef typename getInputChannelTypeByID< ID >::type ChannelType;

		// get the actual channel instance from the map (a unique_ptr)
		auto channelInstance = boost::fusion::at_key< ChannelType >( mInputChannels ).get();

		// TODO this must never happen --> throw exception to fail in release builds
		//      otherwise we get a segfault when trying to return the channel reference
		assert( channelInstance != nullptr );

		// return a reference to the channel, do not release ownership
		return *channelInstance;
	}


private:

	std::string mName;            /**< the name of the sink */
	InputChannels mInputChannels; /**< all incoming data channels of the sink */
};

} /* end namespace impl */


/**
 * @brief Variadic alias for a sink type.
 *
 * This alias just wraps a variadic list of channel parameters into a type sequence for channel
 * generation in the underlying implementation class.
 *
 * @see @c impl::SinkImpl
 *
 * @tparam InputChannelParameters
 *    a list with all parameters describing the output channels that shall be created for the source
 */
template<
	typename... InputChannelParameters
>
using Sink = impl::SinkImpl< boost::mpl::vector< InputChannelParameters... > >;


#endif /* SINK_HPP_ */
