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
 * Source.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: fbeier
 */

#ifndef SOURCE_HPP_
#define SOURCE_HPP_

#include "signals/DefaultSourceSignal.hpp"
#include "channels/parameters/OutputChannelParameters.hpp"
#include "channels/impl/GenerateOutputChannelGroup.hpp"
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
#include "SourceHelperMacros.hpp"


namespace channels {

	/**
	 * @brief A type representing a parameter for creating an output channel using
	 *        the @c DefaultSourceSignal.
	 *
	 * @author Felix Beier <felix.beier@tu-ilmenau.de>
	 */
	template<
		typename... ProducedTypes
	> struct Out :
		public OutputChannelParameters<
			DefaultSourceSignal,
			ProducedTypes...
		>
	{};

} /* end namespace channels */


namespace impl {

/**
 * @brief An interface for producing data elements.
 *
 * A @c Source provides the interface for a publisher in a publish-subscribe pattern.
 * It wraps a group of @c OutputChannels for receiving data, one for each independent
 * set of data element types that shall be published. The channel implementation to
 * be created is described by a set of @c OutputChannelParameters which is passed as
 * template argument list.
 *
 * To receive published data elements, the @c OutputChannels can be subscribed by
 * matching (same or convertible list of types in the signature) @c InputChannels
 * which are, e.g., provided by @c Sink's.
 * Two channels can be connected via the @c connectChannels() helper method where
 * each @c OutputChannel can have multiple outgoing connections, either to the same or
 * to different @c InputputChannels as long as their signatures match.
 *
 * Each @c OutputChannel can be separately accessed with the @c getOutputChannelByID()
 * method, its type via the nested @c getOutputChannelTypeByID meta function.
 * Both require a @c ChannelID as static argument. This ID is generated from the order
 * in which the respective channel was declared. To simplify the usage, some helper
 * macros and structures exist for generating necessary boilerplate code.
 *
 * Subclasses of the @c Source do not need to override anything. This base class just
 * generates all @c OutputChannels as publisher interfaces.
 *
 * @note The @c Source's @c OutputChannels are considered to model independent outgoing
 *       data flows of specific sets of elements where each set is treated as atomic unit
 *       like a tuple. This does NOT model the control flow, i.e., how, in which order,
 *       or by which thread registered subscriber callbacks are invoked. This depends on
 *       the actual implementation within the channel. This behavior can be configured
 *       with specifying a @c SignalImpl policy in the channel parameters.
 *
 * For a comprehensive example, have a look at the @c Sink documentation.
 *
 * @tparam OuputChannelParameters
 *    a type sequence with all parameters describing the output channels that shall be
 *    created for the source
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename OutputChannelParameters
>
class SourceImpl :
	private boost::noncopyable
{
private:

	/// a group of all @c OutputChannel types for this sink
	typedef typename impl::generateOutputChannelGroup<
		SourceImpl, OutputChannelParameters
	>::type OutputChannelGroup;

	/// a list with all output channel types
	typedef typename channel_group::getChannels< OutputChannelGroup >::type OutputChannelTypes;

	/// a fusion map for storing an @c OutputChannel instance for each channel type
	typedef typename impl::CreateChannelInstanceTypes< OutputChannelTypes >::type OutputChannels;


public:

	/// the number of outgoing data channels for this source
	static const size_t NUM_OUTPUT_CHANNELS = boost::mpl::size< OutputChannelParameters >::value;

	BOOST_STATIC_ASSERT_MSG( boost::mpl::size< OutputChannelTypes >::value == NUM_OUTPUT_CHANNELS,
			"unexpected number of generated output channel types" );
	BOOST_STATIC_ASSERT_MSG( boost::fusion::result_of::size< OutputChannels >::value == NUM_OUTPUT_CHANNELS,
			"unexpected number of generated output channel instances" );


	/**
	 * @brief Meta function returning the type of a specific @c OutputChannel of the source.
	 *
	 * @tparam ID the ID of the source's requested @c OutputChannel
	 */
	template< ChannelIDValue ID >
	struct getOutputChannelTypeByID {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_OUTPUT_CHANNELS, "illegal channel ID" );
		typedef typename channel_group::getChannel<
			OutputChannelGroup, ChannelID< ID >
		>::type type;
	};


	/**
	 * @brief Create an new source instance.
	 *
	 * @param[in] name
	 *    optional name assigned to the source, default empty
	 */
	SourceImpl( std::string name = "" ):
		mName( std::move(name) ) {

		// TODO remove
		// std::cout << "creating output channels for source'" << mName << "':" << std::endl;

		// apply the creator for each map entry, it updates the mapped runtime
		// channel instance for each type with a newly created instance
		boost::fusion::for_each( mOutputChannels, impl::ChannelCreator< SourceImpl >( *this ) );
	}

	/**
	 * @brief Get the name assigned to the source.
	 *
	 * @return the source's name
	 */
	const std::string& getName() const {
		return mName;
	}

	/**
	 * @brief Get a reference to a specific @c OutputChannel.
	 *
	 * @tparam ID
	 *    a number representing the unique channel ID in [0 ... NUM_INPUT_CHANNELS)
	 * @return a reference to the requested channel
	 */
	template< ChannelIDValue ID >
	typename getOutputChannelTypeByID< ID >::type&
	getOutputChannelByID() const {
		// determine the type of the requested channel as key
		typedef typename getOutputChannelTypeByID< ID >::type ChannelType;

		// get the actual channel instance from the map (a unique_ptr)
		auto channelInstance = boost::fusion::at_key< ChannelType >( mOutputChannels ).get();

		// TODO this must never happen --> throw exception to fail in release builds
		//      otherwise we get a segfault when trying to return the channel reference
		assert( channelInstance != nullptr );

		// return a reference to the channel, do not release ownership
		return *channelInstance;
	}


private:

	std::string mName;              /**< the name of the source */
	OutputChannels mOutputChannels; /**< all outgoing data channels of the source */
};

} /* end namespace impl */


/**
 * @brief Variadic alias for a source type.
 *
 * This alias just wraps a variadic list of channel parameters into a type sequence for channel
 * generation in the underlying implementation class.
 *
 * @see @c impl::SourceImpl
 *
 * @tparam OuputChannelParameters
 *    a list with all parameters describing the output channels that shall be created for the source
 */
template<
	typename... OutputChannelParameters
>
using Source = impl::SourceImpl< boost::mpl::vector< OutputChannelParameters... > >;


#endif /* SOURCE_HPP_ */
