/*
 * SelectOutputChannelParameters.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef SELECTOUTPUTCHANNELPARAMETERS_HPP_
#define SELECTOUTPUTCHANNELPARAMETERS_HPP_

#include "IsOutputChannelParameter.hpp"

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/vector.hpp>


/**
 * @brief Meta function selecting all @c OutputChannelParameters from a type list.
 *
 * @tparam ChannelParameters
 *    the list of type parameters to be filtered
 */
template<
	typename... ChannelParameters
>
struct SelectOutputChannelParameters :
	public boost::mpl::filter_view<
		boost::mpl::vector< ChannelParameters... >,
		isOutputChannelParameter< boost::mpl::_1 >
	>
{};


#endif /* SELECTOUTPUTCHANNELPARAMETERS_HPP_ */
