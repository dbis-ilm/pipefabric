#ifndef SMOOTHFILTER_HH
#define SMOOTHFILTER_HH

#include <assert>

namespace pfabric
{
	/**
	* @brief the class is used for Smooth image filter
	**/

	class SmoothFilter
	{
	public:
		/**
		* @param kernelSize
		*	the size of kernel matrix
		* @param channel
		*	the number of channels of an image
		* @param type
		*	the type of image from OpenCv library (e.g. CV_32FC3, CV_8UC3 ...)
		**/
		SmoothFilter(unsigned short kernelSize, unsigned short ch, int type)
		{
			BaseImageFilter::kernelSize = kernelSize;
			BaseImageFilter::channels = ch;
			BaseImageFilter::type = type;
		}
		/**
		* @tparam T 
		*	is the type of array of pixels. It can be unsigned char, float
		**/
		template<typename T>
		void apply(T* data, size_t rows, size_t cols)
		{
			cv::Mat mat(rows, cols/channels, type , data);
			cv::blur(mat, mat, cv::Size(kernelSize, kernelSize));
		}
	};
}
#endif //SMOOTHFILTER_HH