/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
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
#ifndef TextFileSource_hpp_
#define TextFileSource_hpp_

#include <iostream>
#include <fstream>
#include <memory>

#include "core/Punctuation.hpp"
#include "core/Tuple.hpp"
#include "qop/DataSource.hpp"
#include "pubsub/channels/ConnectChannels.hpp"
#include "qop/BaseOp.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

	typedef Tuple<StringRef> TString;      //< a tuple containing a line of text
	typedef TuplePtr<StringRef> TStringPtr;  //< tuple pointer

/**
 * @brief TextFileSource is a source operator for reading a text file line by
 *        line and producing a stream of tuples.
 *
 * A FileSource is an operator producing a stream of tuples which are extracted from a file.
 * We assume a simple text file where a record is represented by a separate
 * line. The operator produces a stream of @c TStringPtr elements.
 */
class TextFileSource : public DataSource<TStringPtr> {
public:
	PFABRIC_SOURCE_TYPEDEFS(TStringPtr)

	/**
	 * Creates a new FileSource implementation object for reading data from a file and producing tuples.
	 *
	 * @param fname the name of the file we read the data from
	 */
	TextFileSource(const std::string& fname, unsigned long limit = 0);

	/**
	 * Deallocates all resources.
	 */
	~TextFileSource();

	/**
	 * Performs the actual processing by reading the file, parsing the input tuples and send
	 * the to the subscribers. This method has to be invoked explicitly.
	 *
	 * @return the number of tuples produced
	 */
	unsigned long start();

	 const std::string opName() const override { return std::string("TextFileSource"); }

protected:
	/**
	 * Read the tuples from a file using standard IO functions.

	 * @return the number of tuples produced
	 */
	unsigned long readRawFile();

	/**
	 * Read the tuples from a compressed file.
	 *
	 * @return the number of tuples produced
	 */
	unsigned long readCompressedFile();

	/**
	 * Read the tuples from a memory mapped file.
	 *
	 * @return the number of tuples produced
	 */
	unsigned long readMemoryMappedFile();

	/**
	 * Produce a tuple from the textline and send it to the subscribers.
	 *
	 * @param data the string representing the text line
	 */
	void produceTuple(const StringRef& data);

	/**
	 * Produce a punctuation tuple.
	 *
	 * @param pp the punctuation
	 */
	void producePunctuation(PunctuationPtr pp);

	std::string fileName;        //< the name of the file we read the data from
	unsigned long maxTuples;
};

}

#endif
