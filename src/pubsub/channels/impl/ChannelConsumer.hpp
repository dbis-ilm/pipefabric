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
 * ChannelConsumer.hpp
 *
 *  Created on: Feb 2, 2015
 *      Author: fbeier
 */

#ifndef CHANNELCONSUMER_HPP_
#define CHANNELCONSUMER_HPP_


namespace impl {

/**
 * @brief Interface that must be implemented by a consumer component for the data elements
 *        that are published through an @c InputChannel.
 *
 * @tparam InputChannel
 *           the input channel type which forwards data elements received from producers
 */
template<
	typename InputChannel
>
class ChannelConsumer {
	/// the slot type used by the input channel class
	typedef typename InputChannel::Slot Slot;
public:

	/**
	 * @brief Destructor.
	 *
	 * Just an interface, therefore nothing needs to be done.
	 */
	virtual ~ChannelConsumer() {}

	/**
	 * @brief Return the slot that shall be used to handle incoming data elements
	 *        from an @c InputChannel instance.
	 *
	 * @param[in] channel
	 *           a reference to the channel which shall be bound to the slot
	 * @return a slot representing a callback for the channel's incoming data elements
	 */
	virtual Slot bindInputChannel( const InputChannel& channel ) = 0;
};


} /* end namespace impl */



#endif /* CHANNELCONSUMER_HPP_ */
