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
