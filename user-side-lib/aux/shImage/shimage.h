/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shImage.
 *
 * shImage is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shImage is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shImage.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHIMAGE_H_BY_CYCLOPS
#define SHIMAGE_H_BY_CYCLOPS

#define SH_IMAGE_STRINGIFY_(x) #x
#define SH_IMAGE_STRINGIFY(x) SH_IMAGE_STRINGIFY_(x)

#define SH_IMAGE_VERSION_MAJOR 0
#define SH_IMAGE_VERSION_MINOR 1
#define SH_IMAGE_VERSION_REVISION 2
#define SH_IMAGE_VERSION_BUILD 29
#define SH_IMAGE_VERSION SH_IMAGE_STRINGIFY(SH_IMAGE_VERSION_MAJOR)"."\
		SH_IMAGE_STRINGIFY(SH_IMAGE_VERSION_MINOR)"."\
		SH_IMAGE_STRINGIFY(SH_IMAGE_VERSION_REVISION)"."\
		SH_IMAGE_STRINGIFY(SH_IMAGE_VERSION_BUILD)

#define SH_IMAGE_SUCCESS 0
#define SH_IMAGE_FAIL -1
#define SH_IMAGE_FILE_NOT_FOUND -2
#define SH_IMAGE_NO_MEM -3

#include "shBMP.h"

#endif
