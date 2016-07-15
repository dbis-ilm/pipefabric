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

#ifndef AggregateFunc_hpp_
#define AggregateFunc_hpp_


namespace pfabric {

/**
 * @brief The base class for all aggregation functions operating on a data stream.
 *
 * This class provides the common interface for aggregating over a stream of data elements
 * of type @c Tin, returning a result of type @c Tout.
 *
 * TODO Question: Why runtime polymorphism here? Couldn't this be implemented via CRTP?
 * TODO Remark: All implementations call the virtual init in the constructor which is
 *              generally a bad idea since the object instance does not exist completely,
 *              so it might resolve to an incomplete type. CRTP would avoid this.
 */
template<
	typename Tin,
	typename Tres
>
class AggregateFunc {
public:
	typedef Tres ResultType;
	
	AggregateFunc() {
	}

	virtual ~AggregateFunc() {
	}

	virtual void init() = 0;
	virtual void iterate(Tin const& data, bool outdated = false) = 0;
	virtual Tres value() = 0;
};

} /* end namespace pfabric */


#endif
