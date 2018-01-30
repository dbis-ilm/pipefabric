/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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

void Punctuation::serializeToStream(StreamType& res) const {
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
