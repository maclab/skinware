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

#ifndef __SHBMP_H__BY_CYCLOPS__
#define __SHBMP_H__BY_CYCLOPS__

typedef struct
{
	unsigned short int fileType;	// Which is always "BM"
	unsigned int fileSize;
	unsigned short int reserved1;		// Always 0
	unsigned short int reserved2;		// Always 0
	unsigned int bitmapOffset;
} shBMPHeader;

typedef struct
{
	unsigned int infoHeaderSize;
	unsigned int imageWidth;
	unsigned int imageHeight;
	unsigned short int numberOfPlanes;		// Always 1
	unsigned short int numberOfColorBits;
	unsigned int compressionType;
	unsigned int imageSize;
	unsigned int XPixelsPerMeter;
	unsigned int YPixelsPerMeter;
	unsigned int colorsUsed;		// Number of colors used
	unsigned int importantColors;		// Number of "important" colors used
} shBMPInfo;

// Fills header && info, returns an array of colors, in the BGR format. returns '\0' in case of error
// If header or info where '\0', then those information will be thrown away!
unsigned char *shLoadBMP(const char *bmpFile, shBMPHeader *header, shBMPInfo *info);

// Saves a BMP
int shSaveBMP(const char *bmpFile, shBMPHeader *header, shBMPInfo *info, unsigned char *image);

// Saves a BMP (simpler)
int shSaveBMP(const char *bmpFile, int width, int height, unsigned char *image);

#endif
