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

#ifndef punctuation_hpp_
#define punctuation_hpp_

#include "PFabricTypes.hpp"
#include "TimestampHelper.hpp"
#include "serialize.hpp"
#include "ElementSerializable.hpp"

#include <boost/any.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace pfabric {

/**
 * \brief A punctuation represents a special control tuple for signaling.
 *
 * A punctuation is a control tuple sent to subscribers to signal special situations
 * like end-of-stream, end-of-substreams etc. which can be used to implement special semantics in
 * query operators.
 */
class Punctuation: public ElementSerializable {
public:

	/**
	 * Defines possible types for a punctuation tuple.
	 */
	enum PType {
		None           = (0u),      //< none, shouldn't be used
		EndOfStream    = (1u << 0), //< the end of a stream was identified (e.g. EOF)
		EndOfSubStream = (1u << 1), //< the end of a substream was identified
		WindowExpired  = (1u << 2), //< the window was expired (used together with tumbling windows)
		SlideExpired   = (1u << 3), //< ???
		All            = (~0u),     //< all of the above, used for masking
	};


	/**
	 * Create a new punctuation tuple of the given type and with the provided timestamp.
	 */
	Punctuation(PType pt, const boost::any& val, Timestamp ts = TimestampHelper::timestampFromCurrentTime()) :
		mPtype(pt), mData(val), mTstamp(ts) {
	}

	Punctuation(PType pt, Timestamp ts  = TimestampHelper::timestampFromCurrentTime()) :
		mPtype(pt), mTstamp(ts) {
	}

	Punctuation()  {
	}

	/**
	 * Returns the timestamp of the punctuation tuple, i.e. the time of the arrival.
	 *
	 * \return the timestamp
	 */
	const Timestamp& getTimestamp() const {
		return mTstamp;
	}

	/**
	 * Returns the type of the punctuation tuple.
	 *
	 * \return the punctuation type
	 */
	const PType& ptype() const {
		return mPtype;
	}

	boost::posix_time::ptime timestampAsPtime() const;

	/**
	 * Returns the data associated with the punctuation tuple.
	 *
	 * \return the user data associated with the punctuation.
	 */
	const boost::any& data() const	{
		return mData;
	}

	/**
	 * Prints the punctuation in a simple default format to an ostream.
	 *
	 * \param os an std::ostream instance for printing.
	 */
	void print(std::ostream& os) const;

	void serializeToStream(StreamType& res) override;
	void deserializeFromStream(StreamType& res) override;

	void writeToStream(std::ostream& os);

private:
	PType mPtype;      //< the type of punctuation
	boost::any mData;  //< user data associated with the punctuation
	Timestamp mTstamp; //< the timestamp of the punctuation
};

} /* end namespace pquery */

#endif
