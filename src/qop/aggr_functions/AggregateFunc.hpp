/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
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
class AggregateFuncBase {
protected:
  AggregateFuncBase() {}

public:
    virtual ~AggregateFuncBase() {}
    virtual void init() = 0;
 };

typedef AggregateFuncBase* AggregateFuncBasePtr;

template<
	typename Tin,
	typename Tres
>
class AggregateFunc : public AggregateFuncBase {
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
