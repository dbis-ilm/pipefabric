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
