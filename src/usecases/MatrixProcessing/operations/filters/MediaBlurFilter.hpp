#ifndef MEDIABLUR_HH
#define MEDIABLUR_HH

#include "BaseImageFilter.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace pfabric
{
	/**
	* @brief the class is used for Media Blur image filter
	**/
	class MediaBlurFilter : BaseImageFilter
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
		MediaBlurFilter(unsigned short kernelSize, unsigned short ch, int type)	
		{
			BaseImageFilter::kernelSize = kernelSize;
			BaseImageFilter::channels = ch;
			BaseImageFilter::type = type;
		}
		/**
		* @tparam T 
		*	is the type of array of pixels. It can be unsigned char, float
		**/
		void apply(T *data, std::size_t rows, std::size_t cols)
		{
			cv::Mat mat(rows, cols/channels, type , data);
			medianBlur(mat, mat, kernelSize);
		}
	};
}

#endif //MEDIABLUR_HH