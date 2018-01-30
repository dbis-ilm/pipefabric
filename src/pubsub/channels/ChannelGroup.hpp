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
 * ChannelGroup.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: fbeier
 */

#ifndef CHANNELGROUP_HPP_
#define CHANNELGROUP_HPP_

#include "Channel.hpp"

#include "libcpp/mpl/sequences/InsertAssertUnique.hpp"

#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/is_sequence.hpp>


namespace channel_group {

namespace mpl = boost::mpl;


/**
 * @brief Traits of a channel group implementation.
 *
 * This traits class defines all properties that must be provided by a type in order
 * to act as channel group.
 *
 * A channel group provides a collection of channels. Each channel is uniquely
 * identified via a ChannelID, which can be used as key for accessing a specific
 * channel type.
 */
template< typename _ChannelGroup >
struct ChannelGroupTraits {

	/// all channel types in the group
	typedef typename _ChannelGroup::Channels Channels;

	/// an index for accessing a specific channel type via ID
	typedef typename _ChannelGroup::ChannelsByID ChannelsByID;
};


/**
 * @brief Compile-time channel group structure.
 *
 * A compile-time structure representing a channel group. It satisfies the
 * @c ChannelGroupTraits.
 *
 * @tparam _Channels the vector of all channels in the group
 * @tparam _ChannelsByID a map for accessing all channels by ID
 */
template<
	typename _Channels,
	typename _ChannelsByID
>
struct ChannelGroupImpl {
	typedef _Channels Channels;
	typedef _ChannelsByID ChannelsByID;
};


/**
 * @brief Type constructor for a channel group.
 *
 * @tparam _Channels the vector of all channels in the group
 * @tparam _ChannelsByID a map for accessing all channels by ID
 */
template<
	typename _Channels,
	typename _ChannelsByID
>
class createChannelGroup {
public:
	typedef ChannelGroupImpl< _Channels, _ChannelsByID > type;
};

/**
 * @brief Type constructor for an empty channel group.
 */
typedef ChannelGroupImpl< mpl::vector0<>, mpl::map0<> > EmptyChannelGroup;


/**
 * @brief Get a list with all channels in a channel group.
 *
 * @tparam _ChannelGroup the channel group whose channels shall be returned
 */
template< typename _ChannelGroup >
class getChannels {
private:
	/// make sure that the parameter satisfies the interface for a channel group
	typedef ChannelGroupTraits< _ChannelGroup > ChannelGrp;

public:

	/// return the list containing all channels of the group
	typedef typename ChannelGrp::Channels type;
};


/**
 * @brief Get a channel with a specific ID in a channel group.
 *
 * @tparam _ChannelGroup the channel group whose channels shall be returned
 * @tparam _ID the ID of the channel which shall be returned
 */
template<
	typename _ChannelGroup,
	typename _ID
>
class getChannel {
private:
	/// make sure that the parameter satisfies the interface for a channel group
	typedef ChannelGroupTraits< _ChannelGroup > ChannelGrp;

	/// the group's channel index
	typedef typename ChannelGrp::ChannelsByID ChannelsByID;

	// make sure that a channel with the requested ID is available in the group
	BOOST_MPL_ASSERT_MSG(
		(mpl::has_key< ChannelsByID, _ID >::value),
		NO_CHANNEL_FOR_REQUESTED_ID_FOUND,
		( _ID, ChannelGrp )
	);

public:
	/// return the channel at the requested ID
	typedef typename mpl::at< ChannelsByID, _ID >::type type;
};


/**
 * @brief Add a new channel to a channel group.
 *
 * This meta function adds a new channel to an existing channel group.
 * It assures that no duplicate channels are inserted, i.e., no two channels
 * having the same ID exist.
 *
 * @tparam _ChannelGroup the channel group to which the channel should be added
 * @tparam _Channel the channel to be added
 */
template<
	typename _ChannelGroup,
	typename _Channel
>
class addChannel {
private:
	/// make sure that the parameter satisfies the interface for a channel group
	typedef ChannelGroupTraits< _ChannelGroup > ChannelGrp;
	/// make sure that the parameter satisfies the interface for a channel
	typedef ChannelTraits< _Channel > Channel;

	// fetch the old data internal group data structures
	typedef typename Channel::ChannelID ChannelID;
	typedef typename ChannelGrp::ChannelsByID OldChannelsByID;
	typedef typename ChannelGrp::Channels OldChannels;

	// insert the new channel in the internal data structures
	typedef mpl::pair< ChannelID, _Channel > ChannelIDEntry;
	typedef typename ns_mpl::InsertAssertUnique<
		OldChannelsByID, ChannelIDEntry
	>::type NewChannelsByID;
	typedef typename mpl::push_back< OldChannels, _Channel >::type NewChannels;

public:

	/// return a group comprising the modified data structures
	typedef typename createChannelGroup< NewChannels, NewChannelsByID >::type type;
};


/**
 * @brief Meta function for generating a channel group from a sequence of parameters.
 *
 * This meta function will generate a channel group from a list of channel parameters
 * passed as second argument. Therefore, a sequence of @c ChannelIDs is generated for
 * each parameter passed to uniquely identify the new channel in the group which is
 * created as result.
 * The actual channel type is provided by the second argument which must be a meta function
 * class returning the channel type when invoked with the generated @c ChannelID
 * as first argument and a parameter from the list.
 *
 * @tparam ChannelTypeCreator
 *           binary meta function class that when invoked with a @c ChannelID as first
 *           argument and a @c ChannelParameter structure as second argument,
 *           returns a new channel type
 * @tparam ChannelParms
 *           a list of parameters describing the channel types to be created in the group
 */
template<
	typename ChannelTypeCreator,
	typename ChannelParameters
>
class generateChannelGroup {
private:
	BOOST_MPL_ASSERT(( mpl::is_sequence< ChannelParameters > ));
	BOOST_MPL_ASSERT_NOT(( mpl::empty< ChannelParameters > ));

	/// generate a sequence of IDs for each channel parameter
	typedef mpl::range_c< ChannelIDValue, 0, mpl::size< ChannelParameters >::value > ChannelIDs;

public:

	/// generate the result channel group for each pair of <channelID, channel parameter>
	typedef typename mpl::transform<
		ChannelIDs,         // for each ID
		ChannelParameters,  //     and channel parameter
		ChannelTypeCreator, //        create a new channel type
		mpl::inserter<            //  and insert it into a new sequence
			EmptyChannelGroup,    //  starting with an empty channel group,
			addChannel<           //  invoking addChannel for each channel type
				mpl::_1,
				mpl::_2
			>
		>
	>::type type;
};

} /* end namespace channel_group */


#endif /* CHANNELGROUP_HPP_ */
