/*
 * IsOutputChannelParameter.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef ISOUTPUTCHANNELPARAMETER_HPP_
#define ISOUTPUTCHANNELPARAMETER_HPP_

#include "OutputChannelParameters.hpp"

#include <boost/mpl/bool.hpp>
#include <type_traits>

/**
 * @brief Traits class to determine if a given argument describes @c OutputChannelParameters.
 *
 * General case, evaluating to @c false_.
 *
 * @tparam ChannelParameter
 *    the type to be checked
 */
template<
	typename ChannelParameter
>
struct isOutputChannelParameter :
	public std::is_base_of< impl::OutputChannelParameterBase, ChannelParameter >
{};


#endif /* ISOUTPUTCHANNELPARAMETER_HPP_ */
