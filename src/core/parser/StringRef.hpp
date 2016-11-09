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
