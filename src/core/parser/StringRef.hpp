/*
 * StringRef.hpp
 *
 *  Created on: Apr 1, 2015
 *      Author: fbeier
 */

#ifndef STRINGREF_HPP_
#define STRINGREF_HPP_

namespace pfabric {


/**
 * TODO docs
 */
struct StringRef {

	StringRef() :
		begin_( nullptr ), size_( 0 ) {
	}

	StringRef(const char* begin, int size) :
		begin_(begin), size_(size) {
	}

	void setValues(char * begin, int size) {
		begin_ = begin;
		size_ = size;
	}

	const char* begin() {
		return begin_;
	}

	const char* const begin() const {
		return begin_;
	}

	const char* end() {
		return begin_ + size_;
	}

	const char* const end() const {
		return begin_ + size_;
	}

	int size() const {
		return size_;
	}

	const char* begin_;
	int size_;
};


} /* end namespace pfabric */



#endif /* STRINGREF_HPP_ */
