/*
 * BoostSlot.hpp
 *
 *  Created on: Jan 29, 2015
 *      Author: fbeier
 */

#ifndef BOOSTSLOT_HPP_
#define BOOSTSLOT_HPP_


#include <boost/function.hpp>

/**
 * @brief A slot using the @c boost::function as callback.
 *
 * @tparam PublishedTypes
 *           the list of published data types
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template< typename... PublishedTypes >
using BoostSlot = boost::function< void( PublishedTypes... data ) >;


#endif /* BOOSTSLOT_HPP_ */
