/*
 * Copyright (c) 2014 The PipeFabric team,
 *                    All Rights Reserved.
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

	typedef Tuple<StringRef> TString;
	typedef TuplePtr<TString> TStringPtr;

/**
 * \brief a class encapsulating the implementation details of a FileSource, i.e. reading tuple data from a file.
 *
 * FileSourceImpl encapsulates the implementation of the FileSource operator by doing all the file-related and
 * type-agnostic work and using callbacks of the actual FileSource template.
 */
class TextFileSource : public DataSource<TStringPtr> {
public:
	PFABRIC_SOURCE_TYPEDEFS(TStringPtr)

	/**
	 * Creates a new FileSource implementation object for reading data from a file and producing tuples.
	 *
	 * \param fname the name of the file we read the data from
	 * \param num_fields the number of fields to be read
	 * \param sep the field separator
	 * \param tcb a callback ( which is invoked for each read tuple
	 * \param pcb a callback for punctuation
	 */
	TextFileSource(const std::string& fname);

	/**
	 * Deallocates all resources.
	 */
	~TextFileSource();

	/**
	 * Performs the actual processing by reading the file, parsing the input tuples and send
	 * the to the subscribers. This method has to be invoked explicitly.
	 *
	 * \return the number of tuples produced
	 */
	unsigned long start();

protected:
	unsigned long readRawFile();
	//unsigned long readCompressedFile();
	unsigned long readMemoryMappedFile();

	void produceTuple(const StringRef& data);
	void producePunctuation(PunctuationPtr pp);

	std::string fileName;        //< the name of the file we read the data from
};

/**
 * @brief an operator for reading tuples from a file.
 *
 * A FileSource is an operator producing a stream of tuples which are extracted from a file.
 * We assume a record-oriented file, i.e. each line represented a record where the fields are
 * separated by a delimiter string. The operator can also read compressed files (gzip or bzip2),
 * which is detected by appropriate file extensions (gz or bz2).
 * Note, that we assume that the template parameter is of type TuplePtr<T>.
 */


}

#endif
