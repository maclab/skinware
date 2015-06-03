/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of Ngin.
 *
 * Ngin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ngin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ngin.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Ngin_texture.h"
#include "Ngin_base.h"

#define				SHMACROS_LOG_PREFIX			"Ngin"

#include "shmacros.h"

extern bool			g_Ngin_textureEnabled;
extern bool			g_Ngin_initialized;
extern const shNginTexture	*g_Ngin_currentTexture;

shNginTexture::shNginTexture()
{
	texID = NGIN_INVALID_TEXTURE;
	sizeX = 0;
	sizeY = 0;
	origSizeX = 0;
	origSizeY = 0;
	translateX = 0;
	translateY = 0;
}

shNginTexture::shNginTexture(const char *file, int alg)	// Converts textures to RGBA by default
{
	texID = NGIN_INVALID_TEXTURE;
	sizeX = 0;
	sizeY = 0;
	origSizeX = 0;
	origSizeY = 0;
	translateX = 0;
	translateY = 0;
	if (g_Ngin_initialized)
		shNginTLoad(file, alg);
}

shNginTexture::shNginTexture(const unsigned char *file, int alg)
{
	texID = NGIN_INVALID_TEXTURE;
	sizeX = 0;
	sizeY = 0;
	origSizeX = 0;
	origSizeY = 0;
	translateX = 0;
	translateY = 0;
	if (g_Ngin_initialized)
		shNginTLoad((const char *)file, alg);
}

shNginTexture::shNginTexture(const unsigned char *img, unsigned int w, unsigned int h, int m, int alg)
{
	texID = NGIN_INVALID_TEXTURE;
	sizeX = 0;
	sizeY = 0;
	origSizeX = 0;
	origSizeY = 0;
	translateX = 0;
	translateY = 0;
	if (g_Ngin_initialized)
		shNginTLoad(img, w, h, m, alg);
}

shNginTexture::shNginTexture(const shNginTexture &RHS)	// The copy constructor
{
	*this = RHS;
}

shNginTexture::~shNginTexture()
{
	if (shNginTIsLoaded())
		shNginTFree();
}

void shNginTexture::shNginTFree()
{
	glDeleteTextures(1, &texID);
	texID = NGIN_INVALID_TEXTURE;
	sizeX = 0;
	sizeY = 0;
	origSizeX = 0;
	origSizeY = 0;
	translateX = 0;
	translateY = 0;
	if (g_Ngin_currentTexture == this)
		g_Ngin_currentTexture = '\0';
}

void shNginTexture::shNginTLoad(const char *file, int alg)
{
	shBMPInfo info;
	shBMPHeader header;
	unsigned char *image = shLoadBMP(file, &header, &info);
	shNginTLoad(image, info.imageWidth, info.imageHeight, NGIN_BGR, alg);
	if (image != '\0')
		delete[] image;
}

void shNginTexture::shNginTLoad(const unsigned char *file, int alg)
{
	shNginTLoad((const char *)file, alg);
}

void shNginTexture::shNginTLoad(const unsigned char *img, unsigned int w, unsigned int h, int m, int alg)
{
	if (!g_Ngin_initialized || w > (unsigned)1<<31 || h > (unsigned)1<<31 || img == '\0')
						// Sorry! I currently don't support HUGE textures!
	{
		if (g_Ngin_initialized)
			if (img == '\0')
				LOG("'\\0' passed as image, if you opened a file, it probably couldn't be opened");
		return;
	}
	if (shNginTIsLoaded())
		shNginTFree();
	unsigned char *i = shNginConvertImage(img, w, h, m, NGIN_RGBA);
	for (sizeX = 1; sizeX < (int)w; sizeX <<= 1);
	for (sizeY = 1; sizeY < (int)h; sizeY <<= 1);
	if (sizeX != (int)w || sizeY != (int)h)
	{
		// resize the image
		unsigned char *i2 = shNginTStretch(i, w, h, alg);
		delete[] i;
		i = i2;
	}
	origSizeX = w;
	origSizeY = h;

	glGenTextures(1, &texID);
	if (!g_Ngin_textureEnabled)
		glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, i);
	if (!g_Ngin_textureEnabled)
		glDisable(GL_TEXTURE_2D);
	delete[] i;
}

shNginTexture &shNginTexture::operator =(const shNginTexture &RHS)
{
	if (!RHS.shNginTIsLoaded())
	{
		texID = NGIN_INVALID_TEXTURE;
		sizeX = 0;
		sizeY = 0;
		origSizeX = 0;
		origSizeY = 0;
		translateX = 0;
		translateY = 0;
		return *this;
	}
	unsigned char *RHSdata = new unsigned char[RHS.sizeX*RHS.sizeY<<2];
	RHS.shNginTBind();
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, RHSdata);
	shNginTLoad(RHSdata, RHS.sizeX, RHS.sizeY, NGIN_RGBA);
	origSizeX = RHS.origSizeX;
	origSizeY = RHS.origSizeY;
	delete[] RHSdata;
	return *this;
}

bool shNginTexture::shNginTIsLoaded() const
{
	return (texID != NGIN_INVALID_TEXTURE);
}

void shNginTexture::shNginTTextureCoord(float x, float y) const
{
	glTexCoord2f(x+translateX, y+translateY);
}

void shNginTexture::shNginTBind() const
{
	glBindTexture(GL_TEXTURE_2D, texID);
	g_Ngin_currentTexture = this;
}

unsigned char *shNginTexture::shNginTStretch(unsigned char *img, unsigned int width, unsigned int height, int alg)
{
	unsigned char *image = new unsigned char[sizeX*sizeY<<2];
	switch (alg)
	{
		case NGIN_BILINEAR:
			for (int r = 0; r < sizeY; ++r)
			{
				unsigned int rra = (unsigned int)floor((float)r*height/sizeY);
				unsigned int rrb = (unsigned int)ceil((float)r*height/sizeY);
				float y = (float)r*height/sizeY;
				y -= floor(y);
				for (int c = 0; c < sizeX; ++c)
				{
					unsigned int cca = (unsigned int)floor((float)c*width/sizeX);
					unsigned int ccb = (unsigned int)ceil((float)c*width/sizeX);
					if (rrb >= height)
						rrb = rra;
					if (ccb >= width)
						ccb = cca;
					unsigned int posA = (rra*width+cca)<<2;
					unsigned int posB = (rrb*width+cca)<<2;
					unsigned int posC = (rra*width+ccb)<<2;
					unsigned int posD = (rrb*width+ccb)<<2;
					float x = (float)c*width/sizeX;
					x -= floor(x);
					unsigned int pos = (r*sizeX+c)<<2;
					for (unsigned int color = 0; color < 4; ++color)
					{
						unsigned char A = img[posA+color];
						unsigned char B = img[posB+color];
						unsigned char C = img[posC+color];
						unsigned char D = img[posD+color];
						int d = (int)A;
						int c = (int)B-d;
						int b = (int)C-d;
						int a = (int)D-b-c-d;
						image[pos+color] = (unsigned char)((a*x+c)*y+b*x+d);
					}
				}
			}
			break;
		case NGIN_NEAREST:
		default:
			for (int r = 0; r < sizeY; ++r)
			{
				unsigned int rr = (unsigned int)round((float)r*height/sizeY);
				unsigned int row = rr*width;
				for (int c = 0; c < sizeX; ++c)
				{
					unsigned int cc = (unsigned int)round((float)c*width/sizeX);
					unsigned int pos = (r*sizeX+c)<<2;
					unsigned int oldPos = (row+cc)<<2;
					for (unsigned int color = 0; color < 4; ++color)
						image[pos+color] = img[oldPos+color];
				}
			}
			break;
	}
	return image;
}

void shNginTexture::shNginTTranslate(float x, float y)
{
	translateX += x;
	translateY += y;
}

void shNginTexture::shNginTSetTransparent(unsigned char r, unsigned char g, unsigned char b, unsigned char a, int m)
{
	if (!shNginTIsLoaded())
		return;
	int size = sizeX*sizeY<<2;
	unsigned char *data = new unsigned char[size];
	shNginTBind();
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	for (int i = 0; i < size; i += 4)
		if (data[i] == r && data[i+1] == g && data[i+2] == b)
			data[i+3] = a;
	if (m == NGIN_SMOOTH)
	{
		int n[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
		for (int i = 0; i < size; i += 4)
			for (int j = 0; j < 8; ++j)
				if (data[i] == r && data[i+1] == g && data[i+2] == b)
				{
					int other = i+((n[j][0]+n[j][1]*sizeX)<<2);
					if (other >= 0 && other < size && data[other+3] != a)
						data[other+3] = (255+a)/2;
				}
	}
	if (!g_Ngin_textureEnabled)
		glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	if (!g_Ngin_textureEnabled)
		glDisable(GL_TEXTURE_2D);
	delete[] data;
}
