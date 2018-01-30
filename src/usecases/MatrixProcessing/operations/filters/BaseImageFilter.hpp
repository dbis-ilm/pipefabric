/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
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
