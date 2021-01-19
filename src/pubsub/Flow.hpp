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
 * Flow.hpp
 *
 *  Created on: Feb 11, 2015
 *      Author: fbeier
 */

#ifndef FLOW_HPP_
#define FLOW_HPP_

#include "Source.hpp"
#include "Sink.hpp"
#include "channels/parameters/SelectInputChannelParameters.hpp"
#include "channels/parameters/SelectOutputChannelParameters.hpp"

#include <boost/mpl/empty_base.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/empty.hpp>


/**
 * @brief An interface for a data flow.
 *
 * A @c Flow is an interface for a module that requires both, a subscriber interface
 * for receiving incoming data elements that need to be processed, and a publisher
 * interface for notifying following operators about the results. Therefore, the @c Flow
 * is a @c Sink and a @c Source - and provides both interfaces.
 *
 * @see @c Sink and @c Source for a detailed description and requirements for these interfaces.
 *
 * Since both interfaces are inherited, the helper macros for these base types can be
 * reused without any modifications.
 *
 * Example:
 *
 * @code
 *    	// A simple data flow calculating the division of two incoming integer numbers.
 *    	class TestDiv :
 *    		public Flow<
 *      		  channels::SyncIn< int >  // one input channel for numerator    (will become IN 0)
 *      		, channels::Out< int >     // outgoing division result as int    (will become OUT 0)
 *      		, channels::In< int >      // one input channel for denomiator   (will become IN 1)
 *      		, channels::Out< double >  // outgoing division result as double (will become OUT 1)
 *    		>
 *    	{
 *    	private:
 *
 *    		// need an alias for the flow which is both, a source and a sink
 *    		// -> same macros can be reused
 *    		typedef Flow<
 *    			  channels::SyncIn< int >
 *    			, channels::Out< int >
 *    			, channels::In< int >
 *    			, channels::Out< double >
 *    		> FlowBase;
 *
 *    		// assign alias for input channel types inherited from the sink side
 *    		IMPORT_INPUT_CHANNEL_TYPE( FlowBase, 0, Numerator );
 *    		IMPORT_INPUT_CHANNEL_TYPE( FlowBase, 1, Denominator );
 *
 *    	public:
 *
 *    		TestDiv() :
 *    			mWaitingForNumerator( true ),
 *    			mWaitingForDenominator( true ),
 *    			mNumerator(0),
 *    			mDenominator(0) {
 *    		}
 *
 *    		// bind callbacks for each input channel
 *    		BIND_INPUT_CHANNEL_DEFAULT( Numerator, TestDiv, processNumerator );
 *    		BIND_INPUT_CHANNEL_DEFAULT( Denominator, TestDiv, processDenominator );
 *
 *    	private:
 *
 *    		void processNumerator( int n ) {
 *    			mNumerator = n;
 *    			mWaitingForNumerator = false;
 *    			publishResult();
 *    		}
 *
 *    		void processDenominator( int d ) {
 *    			mDenominator = d;
 *    			mWaitingForDenominator = false;
 *    			publishResult();
 *    		}
 *
 *    		void publishResult() {
 *    			if( !mWaitingForNumerator && !mWaitingForDenominator ) {
 *    				// ignore div 0
 *    				PUBLISH( 0, mNumerator/mDenominator );
 *    				PUBLISH( 1, static_cast< double >(mNumerator)/mDenominator );
 *    				mWaitingForNumerator = true;
 *    				mWaitingForDenominator = true;
 *    			}
 *    		}
 *
 *    		bool mWaitingForNumerator;
 *    		bool mWaitingForDenominator;
 *    		int mNumerator;
 *    		int mDenominator;
 *    	};
 *
 *    	// ...
 *
 *    	testSampleFlow() {
 *    		// typedef ... IntSink;    // (having one int input channel)
 *    		// typedef ... DoubleSink; // (having one double input channel)
 *
 *    	    // create a data flow and some sinks
 *    	    TestDiv div;
 *    	    IntSink intSink;
 *    	    DoubleSink doubleSink;
 *
 *    	    // connect them
 *    	    connectChannels( div.getOutputChannelByID<0>(), intSink.getInputChannelByID<0>() );
 *    	    connectChannels( div.getOutputChannelByID<1>(), doubleSink.getInputChannelByID<0>() );
 *
 *    	    // publish some data directly through the flow's input channels
 *    	    // (could be done with separate sources / flows preceding it in the processing chain)
 *    	    auto& numerator = div.getInputChannelByID<0>();
 *    	    auto& denominator = div.getInputChannelByID<1>();
 *    	    numerator.getSlot()( testNumerator );
 *    	    denominator.getSlot()( testDenominator );
 *      }
 * @endcode
 *
 * @tparam ChannelParameters
 *    a list with all parameters describing both, the in- and output data channels of the flow,
 *    no special order is required, other parameter types will be ignored
 */
template<
	typename... ChannelParameters
>
class Flow :
	public impl::SinkImpl< SelectInputChannelParameters< ChannelParameters... > >,
	public impl::SourceImpl< SelectOutputChannelParameters< ChannelParameters... > >
{};


#endif /* FLOW_HPP_ */
