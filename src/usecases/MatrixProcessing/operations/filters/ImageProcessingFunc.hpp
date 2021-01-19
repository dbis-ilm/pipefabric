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

#ifndef IMGPROCESSING_HH
#define IMGPROCESSING_HH

#include "core/Tuple.hpp"
#include "core/StreamElementTraits.hpp"

namespace pfabric
{
	template<typename Tin, typename ImgAggr>
	class ImageAggregate
	{
	private:
		typedef typename Tin::element_type::template getAttributeType<2>::type ValueType;
		typedef Tin StreamElement;

		// A matrix
		ValueType values; 
		ImgAggr aggr;
	public:
		typedef ValueType ResultType;
		ImageAggregate()	
		{}

		void init(){ }
		void iterate(StreamElement const &rec, bool outdated = false)
		{
			auto rows = get<0>(rec);
			auto cols = get<1>(rec);
			
			values = get<2>(rec);
			aggr(values.getRawData(), rows, cols);
		}
		ResultType value()
		{
			return values;
		}
	};
}



#endif //IMGPROCESSING_HH
