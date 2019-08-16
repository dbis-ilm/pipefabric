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

#ifndef READERSTREAM_HH
#define READERSTREAM_HH

namespace pfabric {

	template<typename InputType>
	class ReaderValue
	{

	public:
		typedef InputType StreamElement;

		template<typename T>
		void insert(const InputType &rec, T *matrix)
		{
			auto x = get<0>(rec);
			auto y = get<1>(rec);
			auto v = get<2>(rec);
			assert(x >= 0 && y >= 0);
			matrix->set(x, y, v);

		}
		template<typename T>
		void erase(const InputType &rec, T *matrix)
		{
			auto x = get<0>(rec);
			auto y = get<1>(rec);
			assert(x >= 0 && y >= 0);
			matrix->remove(x, y);
		}
	};
} //pfabric

#endif //READERSTREAM_HH
