/*
 * Copyright (c) 2014 The PipeFabric team,
 *                    All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabcric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */
#ifndef FileWriter_hpp_
#define FileWriter_hpp_

#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"
#include "3rdparty/format/format.hpp"

#include <iostream>
#include <fstream>
#include <boost/core/ignore_unused.hpp>


namespace pfabric {

/**
 * @brief TODO Doc
 *
 * TODO separate console writer from file writer!
 *      (+common factory interface if required by PipeFlow)
 * TODO extract formatter into separate functor classes (CSV, XML, binary, ...)
 *      and pass as template argument if static / constructor arg if dynamic
 *
 * @tparam StreamElement
 *    the data stream element type consumed by the sink
 */
template<
	typename StreamElement
>
class FileWriter :
	// default sink interface, synchronizing concurrent write attempts
	public SynchronizedDataSink< StreamElement >
{
private:
PFABRIC_SYNC_SINK_TYPEDEFS(StreamElement)

	/// TODO Doc
	typedef std::function< void (std::ostream&, const StreamElement&) > FormatterFunc;

	/**
	 * TODO Doc
	 * @param os
	 * @param streamElement
	 */
	static void defaultFormatter( std::ostream& os, const StreamElement& streamElement ) {
		// TODO access via element traits
		os << streamElement << std::endl;
	}


public:

	/**
	 * TODO Doc
	 *
	 * @param fname
	 * @param ffun
	 */
	FileWriter( const std::string& fname, FormatterFunc ffun = defaultFormatter ) :
		mStream(fname.c_str(), std::ofstream::out), mFormatterFunc(ffun) {}

	/**
	 * TODO Doc
	 */
	virtual ~FileWriter() {
		if (mStream.is_open())
			mStream.close();
	}


	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, FileWriter, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, FileWriter, processPunctuation );


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


	// TODO Doc
	std::ofstream mStream;
	FormatterFunc mFormatterFunc;
};

} /* end namespace pquery*/


#endif
