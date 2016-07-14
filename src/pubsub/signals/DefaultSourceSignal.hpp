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
