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
 * DefaultSlotFunction.hpp
 *
 *  Created on: Jan 27, 2015
 *      Author: fbeier
 */

#ifndef DEFAULTSLOTFUNCTION_HPP_
#define DEFAULTSLOTFUNCTION_HPP_

#include "libcpp/types/detail/Function.hpp"

/**
 * @brief A slot using the default @c ns_types::Function as callback.
 *
 * @tparam PublishedTypes
 *           the list of published data types
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template< typename... PublishedTypes >
using DefaultSlotFunction = ns_types::Function< void, PublishedTypes... >;


#endif /* DEFAULTSLOTFUNCTION_HPP_ */
