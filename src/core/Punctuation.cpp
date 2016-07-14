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

#include "TimestampHelper.hpp"
#include "Punctuation.hpp"

using namespace pfabric;

boost::posix_time::ptime Punctuation::timestampAsPtime() const {
	return TimestampHelper::timestampToPtime(mTstamp);
}


void Punctuation::writeToStream(std::ostream& os) {
	// TODO
}

void Punctuation::print(std::ostream& os) const {
	os << "[";
	switch(mPtype) {
	case EndOfStream: os << "EndOfStream"; break;
	case EndOfSubStream: os << "EndOfSubStream"; break;
	case WindowExpired: os << "WindowExpired"; break;
	case SlideExpired: os << "SlideExpired"; break;
	default: os << mPtype; break;
	}
	os << "|" << /*mData << */ "]";
}

void Punctuation::serializeToStream(StreamType& res) {
	serialize(TupleType::Punctuation, res);
	serialize(mTstamp, res);
	serialize(mPtype, res);
	// TODO serialize m_data
}

void Punctuation::deserializeFromStream(StreamType& res) {
		auto it = res.cbegin();
	   /**
	 	 * don't store this information.
	 	 * This is only an indication whether this tuple is normal or punctuation
		 * in case of network communication
	     */
		deserialize<TupleType>(it, res.cend());
		mTstamp = deserialize<Timestamp>(it, res.cend());
		mPtype = deserialize<PType>(it, res.cend());
	// TODO deserialize m_data
}
