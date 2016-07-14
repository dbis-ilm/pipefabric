/*
 * IsInputChannelParameter.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef ISINPUTCHANNELPARAMETER_HPP_
#define ISINPUTCHANNELPARAMETER_HPP_

#include "InputChannelParameters.hpp"

#include <boost/mpl/bool.hpp>
#include <type_traits>

#include "../../Sink.hpp"

/**
 * @brief Traits class to determine if a given argument describes @c InputChannelParameters.
 *
 * General case, evaluating to @c false_.
 *
 * @tparam ChannelParameter
 *    the type to be checked
 */
template<
	typename ChannelParameter
>
struct isInputChannelParameter :
	public std::is_base_of< impl::InputChannelParameterBase, ChannelParameter >
{};

#endif /* ISINPUTCHANNELPARAMETER_HPP_ */
