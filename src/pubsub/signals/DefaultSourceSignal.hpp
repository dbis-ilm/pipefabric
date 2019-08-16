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
 * DefaultSourceSignal.hpp
 *
 *  Created on: Jan 27, 2015
 *      Author: fbeier
 */

#ifndef DEFAULTSOURCESIGNAL_HPP_
#define DEFAULTSOURCESIGNAL_HPP_

#include <functional>

#include "DefaultSlotFunction.hpp"

#include "OneToManySignal.hpp"

/**
 * @brief A signal using the @c OneToManySignal implementation.
 *
 * This type resolves to the non-thread-safe but faster one-to-many signal implementation with
 * default slot argument.
 *
 * @tparam PublishedTypes
 *           the list of published data types
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template< typename... PublishedTypes >
using DefaultSourceSignal = OneToManySignal< DefaultSlotFunction, PublishedTypes... >;


#endif /* DEFAULTSOURCESIGNAL_HPP_ */
