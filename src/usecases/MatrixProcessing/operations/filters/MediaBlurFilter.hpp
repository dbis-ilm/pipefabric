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
