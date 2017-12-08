#ifndef BASEIMGFILTER_HH
#define BASEIMGFILTER_HH

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace pfabric
{
	/**
	* @brief The class holds common parameters for image filters
	**/
	struct BaseImageFilter
	{
		unsigned short kernelSize; 	//< the size of kernel matrix
		unsigned short channels;	//< the number of channels of an image
		int type;					//< the type of image from OpenCv library (e.g. CV_32FC3, CV_8UC3 ...)
	};
}
#endif //BASEIMGFILTER_HH