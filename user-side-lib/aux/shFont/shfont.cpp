/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shFont.
 *
 * shFont is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shFont is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shFont.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <stdio.h>
#include <vector>
#include <shfont_base.h>
#include <shimage.h>
#define SHMACROS_LOG_PREFIX "shFont"
#include <shmacros.h>

using namespace std;

typedef struct letterLocation_
{
	unsigned short int x, y, width, height;
	float tex_x, tex_y, tex_width, tex_height;
} letterLocation;

static letterLocation	_shfont_letters[256];
static unsigned char	_shfont_color[4]			= {0, 0, 0, 255};
static unsigned char	_shfont_shadow_color[4]			= {0, 0, 0, 255};
static bool		_shfont_shadow_full			= false;
static float		_shfont_shadow_dir_x			= 0,
			_shfont_shadow_dir_y			= 0;
static bool		_shfont_shadow_enable			= false;
static unsigned int	_shfont_texture				= 0;
static float		_shfont_spacing_x			= 0.0f,
			_shfont_spacing_y			= 0.0f;
static float		_shfont_scale_x				= 1.0f,
			_shfont_scale_y				= 1.0f;
static bool		_shfont_is_loaded			= 0;
static unsigned int	_shfont_alignment			= SH_FONT_LEFT_ALIGN | SH_FONT_TOP_ALIGN;
static unsigned char	*_shfont_temp_buffer			= '\0';
static unsigned int	_shfont_temp_buffer_size		= 0;

#define			IS_LEFT_ALIGNED				((_shfont_alignment & SH_FONT_CENTER_ALIGN) == SH_FONT_LEFT_ALIGN)
#define			IS_RIGHT_ALIGNED			((_shfont_alignment & SH_FONT_CENTER_ALIGN) == SH_FONT_RIGHT_ALIGN)
#define			IS_CENTER_ALIGNED			((_shfont_alignment & SH_FONT_CENTER_ALIGN) == SH_FONT_CENTER_ALIGN)
#define			IS_TOP_ALIGNED				((_shfont_alignment & SH_FONT_VERTICAL_CENTER_ALIGN) == SH_FONT_TOP_ALIGN)
#define			IS_BOTTOM_ALIGNED			((_shfont_alignment & SH_FONT_VERTICAL_CENTER_ALIGN) == SH_FONT_BOTTOM_ALIGN)
#define			IS_VERTICAL_CENTER_ALIGNED		((_shfont_alignment & SH_FONT_VERTICAL_CENTER_ALIGN) == SH_FONT_VERTICAL_CENTER_ALIGN)

#define			VALUE_IS_ZERO(x)			((x) < 1e-6 && (x) > -1e-6)

#define			SHADOW_DISPLACEMENT			0.01

// File and memory operations
int shFontConvertToFont(const char *bmpFile, const char *fontFile)	// When writing font, between each letter, puts one line transparent (like in the converToBMP) and puts in the
									// beginning and end of it also. This is so that floating point roundings doesn't get the characters mixed
{
	shBMPHeader bmph;
	shBMPInfo bmpi;
	unsigned char *bmp;
	OPEN_LOG("run.log");
	bmp = shLoadBMP(bmpFile, &bmph, &bmpi);
	if (!bmp)
	{
		LOG("Could not open bmp font file: %s", bmpFile);
		CLOSE_LOG();
		return SH_FONT_FILE_NOT_FOUND;
	}
	FILE *f = fopen(fontFile, "wb");
	if (f == '\0')
	{
		LOG("Could not open file: %s to write font.", fontFile);
		delete[] bmp;
		CLOSE_LOG();
		return SH_FONT_FAIL;
	}
#define IS_RED(x, y) (bmp[(y)*rowBytes+(x)*3+2] == 255 && bmp[(y)*rowBytes+(x)*3+1] == 0 && bmp[(y)*rowBytes+(x)*3] == 0)
																// format is BGR
	vector<letterLocation> lettersInBMP;
	int rowBytes = bmpi.imageWidth*3;
	if (rowBytes%4)
		rowBytes += 4-rowBytes%4;		// 32bit alignment.	// TODO: On 64bit architecture, is this 64bit alignment?
	for (int y = bmpi.imageHeight-1; y >= 0; --y)
		for (unsigned int x = 0; x < bmpi.imageWidth; ++x)
		{
			if (!IS_RED(x, y))
			{
				bool isAlreadySeen = false;
				for (unsigned int i = 0; i < lettersInBMP.size(); ++i)
					if (lettersInBMP[i].x <= x && lettersInBMP[i].y <= bmpi.imageHeight-1-y &&
						lettersInBMP[i].x+lettersInBMP[i].width > x && lettersInBMP[i].y+lettersInBMP[i].height > bmpi.imageHeight-1-y)
					{
						isAlreadySeen = true;
						x = lettersInBMP[i].x+lettersInBMP[i].width;	// Jump over the letter
						break;
					}
				if (isAlreadySeen)
					continue;
				letterLocation newLetter = {x, bmpi.imageHeight-1-y};
				for (newLetter.width = 0;
					newLetter.x+newLetter.width < bmpi.imageWidth && !IS_RED(x+newLetter.width, y);
					++newLetter.width);
				for (newLetter.height = 0;
					newLetter.y+newLetter.height < bmpi.imageHeight && !IS_RED(x, y-newLetter.height);
					++newLetter.height);
				lettersInBMP.push_back(newLetter);
			}
		}
	int rows = 1;					// make sure width doesn't go above 1024 (otherwise some graphics cards don't support)
	int maxHeight = 0;
	int accumulatedWidth = 1;
	int maxAccumulatedWidth = 0;
	if (lettersInBMP.size() > 256)
		LOG("Warning: Detected more than 256 characters in the bmp file \"%s\". Ignoring the rest of them", bmpFile);
	for (unsigned int i = 0; i < lettersInBMP.size() && i < 256; ++i)	// in case there was more than 256 characters, ignore the rest
	{
		if (accumulatedWidth+lettersInBMP[i].width > 1024)
		{
			if (accumulatedWidth > maxAccumulatedWidth)
				maxAccumulatedWidth = accumulatedWidth;
			accumulatedWidth = 1;
			++rows;
		}
		if (maxHeight < lettersInBMP[i].height)
			maxHeight = lettersInBMP[i].height;
		accumulatedWidth += lettersInBMP[i].width + 1;
	}
	if (accumulatedWidth > maxAccumulatedWidth)
		maxAccumulatedWidth = accumulatedWidth;
	accumulatedWidth = 1;
	int curRow = 0;
	vector<int> rowBeginnings;
	rowBeginnings.push_back(0);
	for (unsigned int i = 0; i < lettersInBMP.size() && i < 256; ++i)	// in case there was more than 256 characters, ignore the rest
	{
		if (accumulatedWidth+lettersInBMP[i].width > 1024)
		{
			rowBeginnings.push_back(i);
			accumulatedWidth = 1;
			++curRow;
		}
		int x = accumulatedWidth;				// Leave place for 1 line of transparent on the start, end and between each letter
		int y = curRow * (maxHeight + 1) + 1;			// Leave place for 1 line of transparent on the start, end and between each letter
		fwrite(&x, 4, 1, f);
		fwrite(&y, 4, 1, f);
		fwrite(&lettersInBMP[i].width, 2, 1, f);
		fwrite(&lettersInBMP[i].height, 2, 1, f);
		accumulatedWidth += lettersInBMP[i].width + 1;
	}
	for (unsigned int i = lettersInBMP.size(); i < 256; ++i)	// For missing characters
	{
		int nothing[] = {0, 0, 0};
		fwrite(nothing, 4, 3, f);
	}
	int textureHeight = 1, textureWidth = 1;
	while (textureWidth < maxAccumulatedWidth) textureWidth <<= 1;
	while (textureHeight < rows*(maxHeight+1)+1) textureHeight <<= 1;	// one row on top, bottom and between each row for transparent lines
	LOG("%d %d", textureWidth, textureHeight);
	fwrite(&textureWidth, 4, 1, f);
	fwrite(&textureHeight, 4, 1, f);
	for (int x = 0; x < textureWidth; ++x)
		fputc(0, f);
	curRow = 0;
	accumulatedWidth = 1;
	for (int r = 0; r < rows; ++r)
	{
		unsigned int rowEnd;
		if (r == rows - 1)
			rowEnd = lettersInBMP.size();
		else
			rowEnd = rowBeginnings[r + 1];
		if (rowEnd > 256)
			rowEnd = 256;
		for (int y = 0; y < maxHeight; ++y)			// Write the font image row by row
		{
			int currentWidth = 0;
			fputc(0, f);
			++currentWidth;
			for (unsigned int i = rowBeginnings[r]; i < rowEnd; ++i)
			{
				if (y >= lettersInBMP[i].height)
					for (int x = 0; x < lettersInBMP[i].width; ++x)
						fputc(0, f);		// vertical padding for short letters
				else
					for (int x = 0; x < lettersInBMP[i].width; ++x)
					{
						unsigned char grayscale = bmp[(bmpi.imageHeight-1-lettersInBMP[i].y-y)*rowBytes+(lettersInBMP[i].x+x)*3]/3+
								bmp[(bmpi.imageHeight-1-lettersInBMP[i].y-y)*rowBytes+(lettersInBMP[i].x+x)*3+1]/3+
								bmp[(bmpi.imageHeight-1-lettersInBMP[i].y-y)*rowBytes+(lettersInBMP[i].x+x)*3+2]/3;
						fputc(255-grayscale, f);
					}
				fputc(0, f);
				currentWidth += lettersInBMP[i].width+1;
			}
			for (int i = currentWidth; i < textureWidth; ++i)
				fputc(0, f);	// padding
		}
		for (int x = 0; x < textureWidth; ++x)
			fputc(0, f);
	}
	for (int y = rows * (maxHeight + 1) + 1; y < textureHeight; ++y)
		for (int x = 0; x < textureWidth; ++x)
			fputc(0, f);	// padding
	delete[] bmp;
	fclose(f);
	CLOSE_LOG();
	return SH_FONT_SUCCESS;
}

int shFontConvertToBMP(const char *fontFile, const char *bmpFile)
{
	OPEN_LOG("run.log");
	FILE *f = fopen(fontFile, "rb");
	if (f == '\0')
	{
		LOG("Could not open file \"%s\" to read font.", fontFile);
		CLOSE_LOG();
		return SH_FONT_FILE_NOT_FOUND;
	}
	vector<letterLocation> lettersInFont;
	for (int i = 0; i < 256; ++i)
	{
		letterLocation letter;
		fread(&letter.x, 4, 1, f);
		fread(&letter.y, 4, 1, f);
		fread(&letter.width, 2, 1, f);
		fread(&letter.height, 2, 1, f);
		lettersInFont.push_back(letter);
	}
	int imageWidth, imageHeight;
	fread(&imageWidth, 4, 1, f);
	fread(&imageHeight, 4, 1, f);
	LOG("%d %d", imageWidth, imageHeight);
	if (feof(f))
	{
		LOG("Font file \"%s\" is incomplete", fontFile);
		fclose(f);
		CLOSE_LOG();
		return SH_FONT_FAIL;
	}
	int rowBytes = imageWidth*3;
	if (rowBytes%4)
		rowBytes += 4-rowBytes%4;
	unsigned char *fc = new unsigned char[imageWidth * imageHeight];
	unsigned char *bmpImage = new unsigned char[imageHeight*rowBytes];
	if (fc == NULL || bmpImage == NULL)
	{
		LOG("Out of memory");
		fclose(f);
		if (fc)
			delete[] fc;
		if (bmpImage)
			delete[] bmpImage;
		CLOSE_LOG();
		return SH_FONT_NO_MEM;
	}
	unsigned int read = 0;
	unsigned int length = imageWidth * imageHeight;
	while (read < length)
	{
		unsigned int toRead = 4096;
		if (length - read < 4096)
			toRead = length - read;
		if (fread(fc + read, 1, toRead, f) != toRead)
		{
			LOG("Failed to read file");
			fclose(f);
			delete[] bmpImage;
			delete[] fc;
			CLOSE_LOG();
			return SH_FONT_FAIL;
		}
		read += toRead;
	}
	for (int y = 0; y < imageHeight; ++y)
		for (int x = 0; x < imageWidth; ++x)
		{
			bmpImage[y*rowBytes+x*3+2] = 255;
			bmpImage[y*rowBytes+x*3+1] = 0;
			bmpImage[y*rowBytes+x*3] = 0;
		}
	for (unsigned int i = 0; i < lettersInFont.size(); ++i)
	{
		for (int y = 0; y < lettersInFont[i].height; ++y)
			for (int x = 0; x < lettersInFont[i].width; ++x)
			{
				int ly = y+lettersInFont[i].y;
				int lx = x+lettersInFont[i].x;
				bmpImage[(imageHeight-1-ly)*rowBytes+lx*3+2] = 255-fc[ly*imageWidth+lx];
				bmpImage[(imageHeight-1-ly)*rowBytes+lx*3+1] = 255-fc[ly*imageWidth+lx];
				bmpImage[(imageHeight-1-ly)*rowBytes+lx*3] = 255-fc[ly*imageWidth+lx];
			}
	}
	int ret = shSaveBMP(bmpFile, imageWidth, imageHeight, bmpImage);
	delete[] fc;
	delete[] bmpImage;
	fclose(f);
	CLOSE_LOG();
	return (ret == SH_IMAGE_SUCCESS)?SH_FONT_SUCCESS:SH_FONT_FAIL;
}

int shFontLoad(const char *fontFile)
{
	if (_shfont_texture != 0)
		return SH_FONT_ALREADY_LOADED;
	OPEN_LOG("run.log");
	FILE *f = fopen(fontFile, "rb");
	if (f == '\0')
	{
		LOG("Could not open file \"%s\" to read font.", fontFile);
		return SH_FONT_FILE_NOT_FOUND;
	}
	for (int i = 0; i < 256; ++i)
	{
		fread(&_shfont_letters[i].x, 4, 1, f);
		fread(&_shfont_letters[i].y, 4, 1, f);
		fread(&_shfont_letters[i].width, 2, 1, f);
		fread(&_shfont_letters[i].height, 2, 1, f);
	}
	int imageWidth, imageHeight;
	fread(&imageWidth, 4, 1, f);
	fread(&imageHeight, 4, 1, f);
	if (feof(f))
	{
		LOG("Font file \"%s\" is incomplete", fontFile);
		fclose(f);
		return SH_FONT_FAIL;
	}
	if (imageWidth == 0 || imageHeight == 0)
	{
		LOG("Font file \"%s\" is corrupt", fontFile);
		fclose(f);
		return SH_FONT_FAIL;
	}
	unsigned char *texture = new unsigned char[imageHeight*imageWidth*4];
	if (!texture)
	{
		LOG("Could not acquire memory for font");
		fclose(f);
		return SH_FONT_NO_MEM;
	}
	memset(texture, -1, imageHeight*imageWidth*4*sizeof(char));
	for (int i = 3; i < imageHeight*imageWidth*4; i += 4)
	{
		texture[i] = (unsigned char)fgetc(f);
		if (feof(f))
		{
			LOG("Font file \"%s\" is incomplete", fontFile);
			delete[] texture;
			fclose(f);
			return SH_FONT_FAIL;
		}
	}
	fclose(f);
	glGenTextures(1, &_shfont_texture);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _shfont_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, texture);
	delete[] texture;
	for (int i = 0; i < 256; ++i)
	{
		_shfont_letters[i].tex_x = _shfont_letters[i].x/(float)imageWidth;
		_shfont_letters[i].tex_y = _shfont_letters[i].y/(float)imageHeight;
		_shfont_letters[i].tex_width = _shfont_letters[i].width/(float)imageWidth;
		_shfont_letters[i].tex_height = _shfont_letters[i].height/(float)imageHeight;
	}
	_shfont_is_loaded = true;
	return SH_FONT_SUCCESS;
}

void shFontFree()
{
	glDeleteTextures(1, &_shfont_texture);
	_shfont_is_loaded = 0;
	_shfont_texture = 0;
	_shfont_color[0] = _shfont_color[1] = _shfont_color[2] = 0; _shfont_color[0] = 255;
	_shfont_texture = 0;
	_shfont_spacing_x = 0.0f,
	_shfont_spacing_y = 0.0f;
	_shfont_scale_x = 1.0f,
	_shfont_scale_y = 1.0f;
	_shfont_shadow_enable = false;
	_shfont_shadow_color[0] = _shfont_shadow_color[1] = _shfont_shadow_color[2] = 0; _shfont_shadow_color[3] = 255;
	_shfont_shadow_dir_x = 0.0f;
	_shfont_shadow_dir_y = 0.0f;
	_shfont_shadow_full = false;
	_shfont_alignment = SH_FONT_LEFT_ALIGN | SH_FONT_TOP_ALIGN;
	if (_shfont_temp_buffer)
		delete[] _shfont_temp_buffer;
	_shfont_temp_buffer = '\0';
	_shfont_temp_buffer_size = 0;
	CLOSE_LOG();
	// TODO: reset other variables
}

bool shFontIsLoaded()
{
	return _shfont_is_loaded;
}

// Font rendering
void _shfont_write(int *windowFrame, const unsigned char *text)		// bind texture on every call.
{
	bool texEnabled;
	if (!(texEnabled = glIsEnabled(GL_TEXTURE_2D)))				// Tested with 20000 writes of "No messages yet", detecting and enabling GL_TEXTURE_2D decreased the frame rate from 18 to 17.
		glEnable(GL_TEXTURE_2D);					// My decision is that this is acceptable.
	float currentX = 0.0f, currentY = 0.0f;
	float textWidth, textHeight;
	shFontTextDimensions((char *)text, _shfont_scale_x, _shfont_scale_y, &textWidth, &textHeight);
	if (windowFrame != '\0')
	{
		if (IS_BOTTOM_ALIGNED)
			currentY = windowFrame[1]+textHeight;
		else if (IS_VERTICAL_CENTER_ALIGNED)
			currentY = (windowFrame[3]+windowFrame[1]+textHeight)/2.0f;
	}
	glBindTexture(GL_TEXTURE_2D, _shfont_texture);
	bool first = true;
	glBegin(GL_QUADS);
	for (const unsigned char *l = text; *l; ++l)
	{
		if (!first)
			currentX += _shfont_spacing_x*_shfont_scale_x;
		else if (IS_RIGHT_ALIGNED)
			currentX = textWidth-shFontLineWidth((char *)l, _shfont_scale_x);
		else if (IS_CENTER_ALIGNED)
			currentX = (textWidth-shFontLineWidth((char *)l, _shfont_scale_x))/2.0f;
		first = false;
		if (*l == '\n' || *l == '\r')
		{
			currentX = 0;
			first = true;
			currentY -= (_shfont_letters[*l].height+_shfont_spacing_y)*_shfont_scale_y;		// The heights of characters \n and \r show how much the next line is moved down
		}
		else if (*l == '\t')
		{
			float tabWidth = _shfont_letters[*l].width*_shfont_scale_x;
			currentX += tabWidth;
			currentX -= currentX-floor(currentX/tabWidth)*tabWidth;					// Align to multiples of tabWidth
		}
		// TODO: Any other special characters I want to handle?
		else if (_shfont_letters[*l].width && _shfont_letters[*l].height)
		{
			// Note: the font is stored upside down
			float topright_x = currentX+_shfont_letters[*l].width*_shfont_scale_x;
			float bottomright_y = currentY-_shfont_letters[*l].height*_shfont_scale_y;
			// If shadow was enabled, draw it. It makes more sense to draw it after (since a lot of depth tests will fail, but depth test may not be enabled)
			if (_shfont_shadow_enable)
			{
				float shadow_dir_scaled_x = _shfont_shadow_dir_x*_shfont_scale_x;
				float shadow_dir_scaled_y = _shfont_shadow_dir_y*_shfont_scale_y;
				glColor4ubv(_shfont_shadow_color);
				if (_shfont_shadow_full)
				{
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(currentX+shadow_dir_scaled_x, bottomright_y, -SHADOW_DISPLACEMENT * 4);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(topright_x+shadow_dir_scaled_x, bottomright_y, -SHADOW_DISPLACEMENT * 4);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y);
					glVertex3f(topright_x+shadow_dir_scaled_x, currentY, -SHADOW_DISPLACEMENT * 4);
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y);
					glVertex3f(currentX+shadow_dir_scaled_x, currentY, -SHADOW_DISPLACEMENT * 4);

					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(currentX-shadow_dir_scaled_x, bottomright_y, -SHADOW_DISPLACEMENT * 3);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(topright_x-shadow_dir_scaled_x, bottomright_y, -SHADOW_DISPLACEMENT * 3);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y);
					glVertex3f(topright_x-shadow_dir_scaled_x, currentY, -SHADOW_DISPLACEMENT * 3);
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y);
					glVertex3f(currentX-shadow_dir_scaled_x, currentY, -SHADOW_DISPLACEMENT * 3);

					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(currentX, bottomright_y-shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 2);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(topright_x, bottomright_y-shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 2);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y);
					glVertex3f(topright_x, currentY-shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 2);
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y);
					glVertex3f(currentX, currentY-shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 2);

					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(currentX, bottomright_y+shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 1);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(topright_x, bottomright_y+shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 1);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y);
					glVertex3f(topright_x, currentY+shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 1);
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y);
					glVertex3f(currentX, currentY+shadow_dir_scaled_y, -SHADOW_DISPLACEMENT * 1);
				}
				else
				{
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(currentX+_shfont_shadow_dir_x, bottomright_y+_shfont_shadow_dir_y, -SHADOW_DISPLACEMENT);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
					glVertex3f(topright_x+_shfont_shadow_dir_x, bottomright_y+_shfont_shadow_dir_y, -SHADOW_DISPLACEMENT);
					glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y);
					glVertex3f(topright_x+_shfont_shadow_dir_x, currentY+_shfont_shadow_dir_y, -SHADOW_DISPLACEMENT);
					glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y);
					glVertex3f(currentX+_shfont_shadow_dir_x, currentY+_shfont_shadow_dir_y, -SHADOW_DISPLACEMENT);
				}
			}
			// Draw the character
			glColor4ubv(_shfont_color);
			glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
			glVertex3f(currentX, bottomright_y, 0);
			glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y+_shfont_letters[*l].tex_height);
			glVertex3f(topright_x, bottomright_y, 0);
			glTexCoord2f(_shfont_letters[*l].tex_x+_shfont_letters[*l].tex_width, _shfont_letters[*l].tex_y);
			glVertex3f(topright_x, currentY, 0);
			glTexCoord2f(_shfont_letters[*l].tex_x, _shfont_letters[*l].tex_y);
			glVertex3f(currentX, currentY, 0);
			currentX += _shfont_letters[*l].width*_shfont_scale_x;
		}
	}
	glEnd();
	if (!texEnabled)
		glDisable(GL_TEXTURE_2D);
}

void shFontWrite(int *windowFrame, const char *text, ...)
{
	va_list args;
	va_start(args, text);
	unsigned int length = vsnprintf('\0', 0, text, args);
	va_end(args);
	if (_shfont_temp_buffer_size < length+1)
	{
		if (_shfont_temp_buffer)
			delete[] _shfont_temp_buffer;
		_shfont_temp_buffer = new unsigned char[length+1];
		if (_shfont_temp_buffer)
			_shfont_temp_buffer_size = length+1;
		else
		{
			_shfont_temp_buffer_size = 0;
			LOG("Could not aquire memory to print message");
			return;
		}
	}
	va_start(args, text);
	vsnprintf((char *)_shfont_temp_buffer, length+1, text, args);
	va_end(args);
	_shfont_write(windowFrame, _shfont_temp_buffer);
}

// Font manipulation
void shFontColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)		// "a" makes the whole text transparent, multiplied by
{
	_shfont_color[0] = r;
	_shfont_color[1] = g;
	_shfont_color[2] = b;
	_shfont_color[3] = a;
}

void shFontColor(unsigned char *rgb, unsigned char a)						// the transparency of the font itself
{
	_shfont_color[0] = rgb[0];
	_shfont_color[1] = rgb[1];
	_shfont_color[2] = rgb[2];
	_shfont_color[3] = a;
}

void shFontSize(float sizeX, float sizeY)
{
	_shfont_scale_x = sizeX;
	if (sizeY < 1e-10 && sizeY > -1e-10)
		_shfont_scale_y = sizeX;
	else
		_shfont_scale_y = sizeY;
}

void shFontShadowColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	_shfont_shadow_color[0] = r;
	_shfont_shadow_color[1] = g;
	_shfont_shadow_color[2] = b;
	_shfont_shadow_color[3] = a;
}

void shFontShadowColor(unsigned char *rgb, unsigned char a)
{
	_shfont_shadow_color[0] = rgb[0];
	_shfont_shadow_color[1] = rgb[1];
	_shfont_shadow_color[2] = rgb[2];
	_shfont_shadow_color[3] = a;
}

void shFontShadow(float directionX, float directionY, bool fullshadow)				// either directly give the shadow direction (if fullshadow
												// is selected, direction is ignored) or use
												// SH_FONT_SHADOW_* or SH_FONT_NO_SHADOW or SH_FONT_FULL_SHADOW
												// as predefined parameters
{
	_shfont_shadow_dir_x = directionX;
	_shfont_shadow_dir_y = directionY;
	_shfont_shadow_full = fullshadow;
	if (IS_ZERO(directionX) && IS_ZERO(directionY))
		_shfont_shadow_enable = false;
	else
		_shfont_shadow_enable = true;
}

void shFontLetterSpacing(float spacing)			// extra space between characters
{
	_shfont_spacing_x = spacing;
}

void shFontLineSpacing(float spacing)			// extra space between lines
{
	_shfont_spacing_y = spacing;
}

void shFontAlignment(unsigned int alignment)				// "or"ed value of SH_FONT_*_ALIGN values, if both left and right, then centers (same for vertical)
{
	_shfont_alignment = alignment;
	if (!(_shfont_alignment & SH_FONT_CENTER_ALIGN))
		_shfont_alignment |= SH_FONT_LEFT_ALIGN;
	if (!(_shfont_alignment & SH_FONT_VERTICAL_CENTER_ALIGN))
		_shfont_alignment |= SH_FONT_TOP_ALIGN;
}

// Font utilities
float shFontTextWidth(const char *txt, float scaleX)		// predicts the width of the text to be written
{
	float currentX = 0.0f;
	bool first = true;
	float maxWidth = 0.0f;
	const unsigned char *text = (const unsigned char *)txt;
	if (VALUE_IS_ZERO(scaleX))
		scaleX = _shfont_scale_x;
	for (const unsigned char *l = text; *l; ++l)
	{
		if (!first)
			currentX += _shfont_spacing_x*scaleX;
		first = false;
		if (*l == '\n' || *l == '\r')
		{
			if (maxWidth < currentX)
				maxWidth = currentX;
			currentX = 0;
			first = true;
		}
		else if (*l == '\t')
		{
			float tabWidth = _shfont_letters[*l].width*scaleX;
			currentX += tabWidth;
			currentX -= currentX-floor(currentX/tabWidth)*tabWidth;				// Align to multiples of tabWidth
		}
		else if (_shfont_letters[*l].width && _shfont_letters[*l].height)
			currentX += _shfont_letters[*l].width*scaleX;
	}
	if (maxWidth < currentX)
		maxWidth = currentX;
	return maxWidth;
}

float shFontTextHeight(const char *txt, float scaleY)		// predicts the height of the text to be written
{
	float currentY = 0.0f;
	float maxHeight = 0.0f;
	const unsigned char *text = (const unsigned char *)txt;
	if (VALUE_IS_ZERO(scaleY))
		scaleY = _shfont_scale_y;
	for (const unsigned char *l = text; *l; ++l)
	{
		if (*l == '\n' || *l == '\r')
		{
			currentY -= (_shfont_letters[*l].height+_shfont_spacing_y)*scaleY;		// The heights of characters \n and \r show how much the next line is moved down
			maxHeight = 0;
		}
		else if (*l == '\t') {}
		else if (_shfont_letters[*l].width && _shfont_letters[*l].height)
		{
			if (maxHeight < _shfont_letters[*l].height*scaleY)
				maxHeight = _shfont_letters[*l].height*scaleY;
		}
	}
	return -currentY+maxHeight;
}

void shFontTextDimensions(const char *txt, float scaleX, float scaleY, float *width, float *height)	// combination of shFontTextWidth and shFontTextHeight
{
	float currentX = 0.0f, currentY = 0.0f;
	bool first = true;
	float maxWidth = 0.0f;
	float maxHeight = 0.0f;
	const unsigned char *text = (const unsigned char *)txt;
	if (VALUE_IS_ZERO(scaleX))
		scaleX = _shfont_scale_x;
	if (VALUE_IS_ZERO(scaleY))
		scaleY = _shfont_scale_y;
	for (const unsigned char *l = text; *l; ++l)
	{
		if (!first)
			currentX += _shfont_spacing_x*scaleX;
		first = false;
		if (*l == '\n' || *l == '\r')
		{
			if (maxWidth < currentX)
				maxWidth = currentX;
			currentX = 0;
			first = true;
			currentY -= (_shfont_letters[*l].height+_shfont_spacing_y)*scaleY;		// The heights of characters \n and \r show how much the next line is moved down
			maxHeight = 0;
		}
		else if (*l == '\t')
		{
			float tabWidth = _shfont_letters[*l].width*scaleX;
			currentX += tabWidth;
			currentX -= currentX-floor(currentX/tabWidth)*tabWidth;				// Align to multiples of tabWidth
		}
		else if (_shfont_letters[*l].width && _shfont_letters[*l].height)
		{
			currentX += _shfont_letters[*l].width*scaleX;
			if (maxHeight < _shfont_letters[*l].height*scaleY)
				maxHeight = _shfont_letters[*l].height*scaleY;
		}
	}
	if (maxWidth < currentX)
		maxWidth = currentX;
	*width = maxWidth;
	*height = -currentY+maxHeight;
}

float shFontLineWidth(const char *txt, float scaleX)		// similar to shFontTextWidth, but predicts the width of only the current line, that is it stops on '\n' or '\r'
{
	float currentX = 0.0f;
	bool first = true;
	float maxWidth = 0.0f;
	const unsigned char *text = (const unsigned char *)txt;
	if (VALUE_IS_ZERO(scaleX))
		scaleX = _shfont_scale_x;
	for (const unsigned char *l = text; *l; ++l)
	{
		if (!first)
			currentX += _shfont_spacing_x*scaleX;
		first = false;
		if (*l == '\n' || *l == '\r')
			break;
		else if (*l == '\t')
		{
			float tabWidth = _shfont_letters[*l].width*scaleX;
			currentX += tabWidth;
			currentX -= currentX-floor(currentX/tabWidth)*tabWidth;				// Align to multiples of tabWidth
		}
		else if (_shfont_letters[*l].width && _shfont_letters[*l].height)
			currentX += _shfont_letters[*l].width*scaleX;
	}
	maxWidth = currentX;
	return maxWidth;
}

float shFontLineHeight(const char *txt, float scaleY)		// similar to shFontTextHeight, but predicts the height of only the current line, that is it stops on '\n' or '\r'
{
	float maxHeight = 0.0f;
	const unsigned char *text = (const unsigned char *)txt;
	if (VALUE_IS_ZERO(scaleY))
		scaleY = _shfont_scale_y;
	for (const unsigned char *l = text; *l; ++l)
	{
		if (*l == '\n' || *l == '\r')
			break;
		else if (*l == '\t') {}
		else if (_shfont_letters[*l].width && _shfont_letters[*l].height)
		{
			if (maxHeight < _shfont_letters[*l].height*scaleY)
				maxHeight = _shfont_letters[*l].height*scaleY;
		}
	}
	return maxHeight;
}
