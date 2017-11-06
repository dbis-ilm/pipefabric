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
