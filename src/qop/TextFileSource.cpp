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

#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>
#ifdef COMPRESSED_FILE_SOURCE
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stdio.h>


//#define POSIX
#include <sys/mman.h>
#ifdef POSIX
#define SHARED  false
#define ADVISE  false
#else
#include <boost/iostreams/device/mapped_file.hpp>
#endif

#include "TextFileSource.hpp"

#define MAX_TUPLE_SIZE 10240

using namespace pfabric;

TextFileSource::TextFileSource(const std::string& fname, unsigned long limit) :
		fileName(fname), maxTuples(limit) {
}

TextFileSource::~TextFileSource() {
}

unsigned long TextFileSource::readRawFile() {
	unsigned long ntuples = 0;
	static const unsigned int BUFFER_SIZE = 16 * 1024;
	int fd = open(fileName.c_str(), O_RDONLY);
	if (fd == -1) {
		BOOST_LOG_TRIVIAL(error) << "TextFileSource::readRawFile: cannot open file '"
		<< fileName << "'.";
		return 0;
	}

	/* Advise the kernel of our access pattern. */
	// posix_fadvise(fd, 0, 0, 1); // FDADVICE_SEQUENTIAL
	char buf[BUFFER_SIZE + 1];
	StringRef data;
	char *store = new char[MAX_TUPLE_SIZE]; // This limits the size of a single tuple!

	char *f;
	size_t remaining = 0, start = 0;
	while (size_t bytes_read = read(fd, buf, BUFFER_SIZE)) {
		if (bytes_read == (size_t) -1) {
			BOOST_LOG_TRIVIAL(error)<< "TextFileSource::readRawFile: error reading file.";
			break;
		}
		if (!bytes_read)
			break;
		for (char *p = buf; (f = (char*) memchr(p, '\n', (buf + bytes_read) - p)); ++p) {
			BOOST_ASSERT_MSG(f - p + 1 + start < MAX_TUPLE_SIZE, "fatal: maximum tuple size exceeded.");
			memcpy(store + start, p, f - p + 1);
			store[f - p + start] = '\0';
			data.setValues(store, f - p + start);
			produceTuple(data);
			p = f;
			if (++ntuples == maxTuples) {
        close(fd);
        delete [] store;
        return ntuples;
      }
			start = 0;
			remaining = p - buf + 1;
		}
		if (remaining < bytes_read) {
			memcpy(store, buf + remaining, bytes_read - remaining);
			start = bytes_read - remaining;
		}
	}
	close(fd);
	delete [] store;
	return ntuples;
}

unsigned long TextFileSource::readMemoryMappedFile() {
	unsigned long ntuples = 0;
	void *f;
	char *p;
#ifdef POSIX
	struct stat st;
	int fd = open(fileName.c_str(), O_RDONLY);
	if (fd == -1) {
		BOOST_LOG_TRIVIAL(error)<< "TextFileSource::readMemoryMappedFile: cannot open file '" << fileName << "'.";
		return 0;
	}
	int status = fstat(fd, &st);
	if (status == -1) {
		BOOST_LOG_TRIVIAL(error)<< "TextFileSource::readMemoryMappedFile: fstat failed.";
		return 0;
	}
	std::size_t size = st.st_size;
#ifdef __linux__
	f = mmap(NULL, size, PROT_READ,
	MAP_FILE | (SHARED ? MAP_SHARED : MAP_PRIVATE) | MAP_POPULATE, fd, 0);
#else
	f = mmap(NULL, size, PROT_READ, MAP_FILE | (SHARED?MAP_SHARED:MAP_PRIVATE), fd, 0);
#endif
	if (f == MAP_FAILED) {
		std::cout << "Data can't be mapped???" << std::endl;
		return -1;
	}
	if (ADVISE)
		if (madvise(f, size, MADV_SEQUENTIAL | MADV_WILLNEED) != 0)
			std::cerr << " Couldn't set hints" << std::endl;
	close(fd);
#else
	boost::iostreams::mapped_file_source file;
	file.open(fileName);
	f = (char*)file.data();
#endif
//char buf[BUFFER_SIZE + 1];
StringRef data;
char *store = new char[MAX_TUPLE_SIZE]; // This limits the size of a single tuple!
char* pch;

for (p = (char*) f; (pch = (char*) strchr(p, '\n')); ++p) {
//	std::fill( store, store + MAX_TUPLE_SIZE, NULL );
	memcpy(store, p, pch - p + 1);
	store[pch - p] = '\0';
	data.setValues(store, pch - p);
	produceTuple(data);
	p = pch;
  if (++ntuples == maxTuples) {
    break;
  }
}

#ifdef POSIX
	munmap(f, size);
#else
	file.close();
#endif
    delete [] store;
	return ntuples;

}
unsigned long TextFileSource::readCompressedFile() {
	std::string line;
	unsigned long ntuples = 0;
	std::ifstream file;
	boost::iostreams::filtering_istream in;

#ifdef COMPRESSED_FILE_SOURCE
	try {
		if (boost::algorithm::iends_with(fileName, ".gz")) {
			file.open(fileName.c_str(),
					std::ios_base::in | std::ios_base::binary);
			in.push(boost::iostreams::gzip_decompressor());
		} else if (boost::algorithm::iends_with(fileName, ".bz2")) {
			file.open(fileName.c_str(),
					std::ios_base::in | std::ios_base::binary);
			in.push(boost::iostreams::bzip2_decompressor());
		} else
			file.open(fileName.c_str());

		in.push(file);

		StringRef data;
		while (getline(in, line)) {
			data.setValues( const_cast<char *> (line.c_str()), line.size());
			produceTuple(data);
      if (++ntuples == maxTuples) {
        break;
      }
    }
		file.close();

	} catch (std::ifstream::failure e) {
		std::cerr << "exception opening/reading file" << std::endl;
	}
#endif
	return ntuples;
}

unsigned long TextFileSource::start() {
	unsigned long ntuples = 0;

	if (boost::algorithm::iends_with(fileName, ".gz")
			|| boost::algorithm::iends_with(fileName, ".bz2"))
		ntuples = readCompressedFile();
	else
  	ntuples = readMemoryMappedFile();
	// publish punctuation
  producePunctuation(std::make_shared<Punctuation>(Punctuation::EndOfStream));
	return ntuples;
}

void TextFileSource::produceTuple(const StringRef& data) {
	auto tn = makeTuplePtr(data);
	this->getOutputDataChannel().publish(tn, false);
}

void TextFileSource::producePunctuation(PunctuationPtr pp) {
	this->getOutputPunctuationChannel().publish(pp);
}
