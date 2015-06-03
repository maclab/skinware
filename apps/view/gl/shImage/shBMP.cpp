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

#include <stdio.h>
#include "shimage.h"
#include "shBMP.h"

unsigned char *shLoadBMP(const char *bmpFile, shBMPHeader *header, shBMPInfo *info)
{
	unsigned char *bitmap;
	shBMPHeader tmpHeader;
	shBMPInfo tmpInfo;
	if (header == '\0')
		header = &tmpHeader;
	if (info == '\0')
		info = &tmpInfo;
	FILE *BMPFile = fopen(bmpFile, "rb");
	if (!BMPFile)
		return '\0';
	fread(&(header->fileType), sizeof(unsigned short int), 1, BMPFile);
	fread(&(header->fileSize), sizeof(unsigned int), 1, BMPFile);
	fread(&(header->reserved1), sizeof(unsigned short int), 1, BMPFile);
	fread(&(header->reserved2), sizeof(unsigned short int), 1, BMPFile);
	fread(&(header->bitmapOffset), sizeof(unsigned int), 1, BMPFile);
	if (header->fileType != 'B'+('M'<<8))//'MB')
	{
		fclose(BMPFile);
		return '\0';
	}
	if (header->reserved1 || header->reserved2)
	{
		fclose(BMPFile);
		return '\0';
	}
	fread(&(info->infoHeaderSize), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->imageWidth), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->imageHeight), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->numberOfPlanes), sizeof(unsigned short int), 1, BMPFile);
	fread(&(info->numberOfColorBits), sizeof(unsigned short int), 1, BMPFile);
	fread(&(info->compressionType), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->imageSize), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->XPixelsPerMeter), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->YPixelsPerMeter), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->colorsUsed), sizeof(unsigned int), 1, BMPFile);
	fread(&(info->importantColors), sizeof(unsigned int), 1, BMPFile);

	bitmap = new unsigned char [header->fileSize-header->bitmapOffset];
	fseek(BMPFile, header->bitmapOffset, SEEK_SET);
	if (fread(bitmap, 1, header->fileSize-header->bitmapOffset, BMPFile) != header->fileSize-header->bitmapOffset)
	{
		fclose(BMPFile);
		delete[] bitmap;
		return '\0';
	}
	fclose(BMPFile);
	return bitmap;
}

int shSaveBMP(const char *bmpFile, shBMPHeader *header, shBMPInfo *info, unsigned char *image)
{
	FILE *BMPFile = fopen(bmpFile, "wb");
	if (!BMPFile)
		return SH_IMAGE_FILE_NOT_FOUND;
	fwrite(&(header->fileType), sizeof(unsigned short int), 1, BMPFile);
	fwrite(&(header->fileSize), sizeof(unsigned int), 1, BMPFile);
	fwrite(&(header->reserved1), sizeof(unsigned short int), 1, BMPFile);
	fwrite(&(header->reserved2), sizeof(unsigned short int), 1, BMPFile);
	fwrite(&(header->bitmapOffset), sizeof(unsigned int), 1, BMPFile);
	fwrite(info, 1, header->bitmapOffset-14, BMPFile);
	fwrite(image, 1, header->fileSize-header->bitmapOffset, BMPFile);
	return SH_IMAGE_SUCCESS;
}

int shSaveBMP(const char *bmpFile, int width, int height, unsigned char *image)
{
	shBMPHeader hdr;
	shBMPInfo info;
	hdr.fileType = 'B'+('M'<<8);//'MB';
	unsigned int rowBytes = width*3;
	if (rowBytes%4)
		rowBytes += 4-rowBytes%4;
	int sizeofhdr = sizeof(hdr.fileType)+sizeof(hdr.fileSize)+sizeof(hdr.reserved1)+sizeof(hdr.reserved2)+sizeof(hdr.bitmapOffset);
	hdr.fileSize = sizeofhdr+sizeof(info)+rowBytes*height;
	hdr.reserved1 = hdr.reserved2 = 0;
	hdr.bitmapOffset = sizeofhdr+sizeof(info);
	info.infoHeaderSize = sizeof(info);
	info.imageWidth = width;
	info.imageHeight = height;
	info.numberOfPlanes = 1;
	info.numberOfColorBits = 24;
	info.imageSize = height*rowBytes;
	info.compressionType = info.XPixelsPerMeter = info.YPixelsPerMeter = 0;
	info.colorsUsed = info.importantColors = 0;
	return shSaveBMP(bmpFile, &hdr, &info, image);
}
