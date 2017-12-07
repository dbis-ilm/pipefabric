#ifndef LAPLACIANFILTER_HH
#define LAPLACIANFILTER_HH

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace pfabric
{
	/**
	* @brief the class is used for Laplacian image filter
	**/
	class LaplacianFilter
	{
		/**
		* @tparam T 
		*	is the type of array of pixels. It can be unsigned char, float
		**/
		template<typename T>
		void apply(T *data) 
		{
			cv::Mat mat(rows, cols/channels, type , data);
			Laplacian(mat, mat, mat.depth());
		}
	};
}

#endif //LAPLACIANFILTER_HH