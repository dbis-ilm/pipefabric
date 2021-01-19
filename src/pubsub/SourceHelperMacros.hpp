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
 * SourceHelperMacros.hpp
 *
 *  Created on: Feb 5, 2015
 *      Author: fbeier
 */

#ifndef SOURCEHELPERMACROS_HPP_
#define SOURCEHELPERMACROS_HPP_


/**
 * @brief Helper macro for importing the type of a specific output channel from a source.
 *
 * This macro can be invoked in classes that inherit from a @c Source in order to
 * import the types of certain @c OutputChannels as type alias.
 *
 * @param sourceType
 *    the alias for the source type used as base class
 * @param id
 *    the unique ID of the output channel whose type shall be imported
 * @param channelName
 *    the type alias to be used for the output channel type
 */
#define IMPORT_OUTPUT_CHANNEL_TYPE( sourceType, id, channelName ) \
	typedef typename sourceType::template getOutputChannelTypeByID< id >::type channelName;


/**
 * @brief Helper macro for publishing data elements through a specific output channel.
 *
 * This macro can be invoked in classes that inherit from a @c Source in order to
 * publish some data through one of its @c OutputChannels identified by the @c channelID.
 *
 * @param channelID
 *    the ID of the @c Source's output channel that shall publish the data
 * @param ...
 *    the list of data elements to be published
 */
#define PUBLISH( channelID, ... ) \
	getOutputChannelByID< channelID >().publish( __VA_ARGS__ );


#endif /* SOURCEHELPERMACROS_HPP_ */
