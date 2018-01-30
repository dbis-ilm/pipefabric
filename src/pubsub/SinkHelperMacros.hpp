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
 * SinkHelperMacros.hpp
 *
 *  Created on: Feb 5, 2015
 *      Author: fbeier
 */

#ifndef SINKHELPERMACROS_HPP_
#define SINKHELPERMACROS_HPP_

#include "libcpp/utilities/BindVariadic.hpp"


/**
 * @brief Helper macro for importing the type of a specific input channel from a sink.
 *
 * This macro can be invoked in classes that inherit from a @c Sink in order to
 * import the types of certain @c InputChannels as type alias.
 *
 * @param sinkType
 *    the alias for the sink type used as base class
 * @param id
 *    the unique ID of the input channel whose type shall be imported
 * @param channelName
 *    the type alias to be used for the input channel type
 */
#define IMPORT_INPUT_CHANNEL_TYPE( sinkType, id, channelName ) \
	typedef typename sinkType::template getInputChannelTypeByID< id >::type channelName;


/**
 * @brief Helper macro for binding a sink's input channel to a member function of the
 *        inheriting class.
 *
 * This macro generates the boilerplate code for binding a specific input channel to
 * a member function that shall handle incoming data elements from the channel.
 * It should be invoked inside the declaration of the class that inherits the sink
 * and must implement (or override in case of deeper inheritance hierarchies)
 * the @c bindInputChannel() method to provide a callback for the channel.
 *
 * It is required that the signature of the @c memberFunction matches the declaration
 * of the channel's published data types, i.e.:
 *   - the method signature must be known at declaration time, i.e., it is not
 *     possible to bind to a generic variadic template member function accepting
 *     a template parameter pack as arguments where the actual signature is determined
 *     when the template function is instantiated
 *   - the order of the types must be the same
 *   - the parameter types in the signature must be the same or be convertible from
 *     the respective type published by the channel
 *   .
 *
 * If binding the callback requires non-default behavior, this macro cannot be used
 * and binding must be done manually, e.g., in case:
 *   - additional arguments need to be injected
 *   - the order of arguments change relative to the channel type declaration
 *   - the method which shall be bound is determined during runtime
 *   - ...
 *   .
 *
 * @param channelName
 *    the type alias to be used for the input channel type, could be generated using
 *    the @c IMPORT_INPUT_CHANNEL_TYPE macro
 * @param className
 *    the name of the class which is declared
 * @param memberFunction
 *    the name of the member function to be bound
 */
#define BIND_INPUT_CHANNEL_DEFAULT( channelName, className, memberFunction ) \
virtual typename channelName::Slot bindInputChannel( const channelName& c ) override { \
	return ns_utilities::bindVariadic< void >( &className::memberFunction, this ); \
}


#endif /* SINKHELPERMACROS_HPP_ */
