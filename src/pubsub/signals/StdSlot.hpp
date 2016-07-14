/*
 * StdSlot.hpp
 *
 *  Created on: Jan 29, 2015
 *      Author: fbeier
 */

#ifndef STDSLOT_HPP_
#define STDSLOT_HPP_

#include <functional>

/**
 * @brief A slot using the @c std::function as callback.
 *
 * @tparam PublishedTypes
 *           the list of published data types
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template< typename... PublishedTypes >
using StdSlot = std::function< void( PublishedTypes... data ) >;


#endif /* STDSLOT_HPP_ */
