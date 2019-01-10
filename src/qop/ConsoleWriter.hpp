/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ConsoleWriter_hpp_
#define ConsoleWriter_hpp_

#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "fmt/format.h"

#include <iostream>
#include <boost/core/ignore_unused.hpp>


namespace pfabric {

/**
 * @brief ConsoleWriter is an operator for printing stream elements to
 * an output stream such as std::cout.
 *
 * ConsoleWriter is an operator for printing stream elements to
 * an output stream. The output stream has to be a subclass of std::ostream
 * (such as cout or cerr) and is assumed to be already opened. In addition
 * to the output stream an optional formatter function can be specified that
 * is used to format the output of each stream element.
 *
 * @tparam StreamElement
 *    the data stream element type consumed by the sink
 */
template<
	typename StreamElement
>
class ConsoleWriter :
	// default sink interface, synchronizing concurrent write attempts
	public SynchronizedDataSink< StreamElement >
{
private:
PFABRIC_SYNC_SINK_TYPEDEFS(StreamElement)

public:
  /// typedef for the formatter function
  typedef std::function< void (std::ostream&, const StreamElement&) > FormatterFunc;

  /**
   * @brief The default formatter function.
	 *
	 * @c defaultFormatter is the default formatter function for @c ConsoleWriter
	 * which simply prints the stream element
	 *
   * @param os the output stream
   * @param streamElement the stream element to be printed
   */
  static void defaultFormatter(std::ostream& os, const StreamElement& streamElement ) {
    // TODO access via element traits
    os << streamElement << std::endl;
  }


	/**
	 * @brief Create a new instance of the ConsoleWriter operator.
	 *
	 * Create a new instance of the ConsoleWriter operator which sends all
	 * incoming and not outdated tuples to the output stream (default std::cout).
	 * Optionally, a formatter function can be specified.
	 *
	 * @param os the output stream (default: std::cout)
	 * @param ffun pointer to the formatter function, if no function is given, the
	 *        default formatter simply prints the stream element.
	 */
	ConsoleWriter(std::ostream& os = std::cout, FormatterFunc ffun = defaultFormatter ) :
		mStream(os), mFormatterFunc(ffun) {
	}


	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, ConsoleWriter, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, ConsoleWriter, processPunctuation );

	const std::string opName() const override { return std::string("ConsoleWriter"); }

private:

	/**
	 * @brief Handle an incoming data stream element.
	 *
	 * This method is invoked when a new stream element arrives. It will be formatted using
	 * the internal formatter which writes it to the standard output stream associated
	 * with this sink instance.
	 *
	 * The incoming @c data element will be ignored if it was flagged as @c outdated.
	 *
	 * @param[in] data
	 *    the data stream element to be written to the output stream
	 * @param[in] outdated
	 *    flag indicating if the @c data element was marked as outdated
	 */
	void processDataElement( const StreamElement& data, const bool outdated ) {
		if( outdated == false ) {
			mFormatterFunc(mStream, data);
		}
	}

	/**
	 * @brief Handle an incoming punctuation tuple.
	 *
	 * Nothing is done here.
	 *
	 * @param[in] punctuation
	 *    the stream punctuation
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		boost::ignore_unused( punctuation );
	}

	std::ostream& mStream;         //< the stream for outputting the stream elements
	FormatterFunc mFormatterFunc;  //< the formatter function
};

}


#endif
