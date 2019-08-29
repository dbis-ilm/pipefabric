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

#ifndef STRINGREF_HPP_
#define STRINGREF_HPP_

#include <string>

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

	const char* begin() const {
		return begin_;
	}

	const char* end() {
		return begin_ + size_;
	}

	const char* end() const {
		return begin_ + size_;
	}

	int size() const {
		return size_;
	}

	const char* begin_;
	int size_;
};

inline bool operator==(const StringRef &a, const StringRef &b) {
  //TODO: Should this be based on pointer and size or content?
  std::string s1 (a.begin_, a.size_);
  std::string s2 (b.begin_, b.size_);
  return (s1 == s2);
}

} /* end namespace pfabric */

#endif /* STRINGREF_HPP_ */
