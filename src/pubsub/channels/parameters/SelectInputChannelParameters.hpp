/*
 * SelectInputChannelParameters.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef SELECTINPUTCHANNELPARAMETERS_HPP_
#define SELECTINPUTCHANNELPARAMETERS_HPP_

#include "IsInputChannelParameter.hpp"

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/quote.hpp>

/**
 * @brief Meta function selecting all @c InputChannelParameters from a type list.
 *
 * @tparam ChannelParameters
 *    the list of type parameters to be filtered
 */
template<
	typename... ChannelParameters
>
struct SelectInputChannelParameters :
	public boost::mpl::filter_view<
		boost::mpl::vector< ChannelParameters... >,
		boost::mpl::quote1< isInputChannelParameter >
	>::type
{};


#endif /* SELECTINPUTCHANNELPARAMETERS_HPP_ */
