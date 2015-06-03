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

#ifndef NGIN_TEXTURE_H_BY_CYCLOPS
#define NGIN_TEXTURE_H_BY_CYCLOPS

#include <shimage.h>
#include "Ngin_opengl.h"

#define			NGIN_NEAREST		0x00
#define			NGIN_BILINEAR		0x01

#define			NGIN_RGB		GL_RGB
#define			NGIN_BGR		GL_BGR_EXT
#define			NGIN_RGBA		GL_RGBA

#define			NGIN_SMOOTH		0x01
#define			NGIN_NOT_SMOOTH		!NGIN_SMOOTH

#define			NGIN_INVALID_TEXTURE	((unsigned int)-1)

class shNginTexture
{
private:
	unsigned int texID;			// The ID that openGL gives to the texture, -1 if image is not loaded!
	unsigned char *shNginTStretch(unsigned char *img, unsigned int width, unsigned int height, int alg);
						// converts from width::height to sizeX::sizeY
public:
	int sizeX;				// The width and height of the texture
	int sizeY;				// If the file given as a texture was not a 2^n*2^m image, the smallest n, m
						// bigger than width and height would be selected and image is stretched so
						// that openGL wouldn't have any problem with it.

	int origSizeX;				// The original width and height of the image
	int origSizeY;				// that this texture was deriven from.

	float translateX, translateY;
	shNginTexture();
	shNginTexture(const char *file, int alg = NGIN_BILINEAR);		// Converts textures to RGBA by default
	shNginTexture(const unsigned char *file, int alg = NGIN_BILINEAR);
	shNginTexture(const unsigned char *img, unsigned int w, unsigned int h, int m = NGIN_BGR, int alg = NGIN_BILINEAR);
	shNginTexture(const shNginTexture &RHS);// The copy constructor
	~shNginTexture();
	void shNginTFree();			// Assumes the texture is already loaded, use shNginTIsLoaded() before
	void shNginTLoad(const char *file, int alg = NGIN_BILINEAR);
	void shNginTLoad(const unsigned char *file, int alg = NGIN_BILINEAR);
	void shNginTLoad(const unsigned char *img, unsigned int w, unsigned int h,
			int m = NGIN_BGR, int alg = NGIN_BILINEAR);
						// The texture would be wrapped around and stretched using either
						// "nearest" or "bilinear" algorithm, default is "nearest"
	shNginTexture &operator =(const shNginTexture &RHS);
	bool shNginTIsLoaded() const;
	void shNginTTextureCoord(float x, float y) const;
						// Assumes the texture is already loaded, use shNginTIsLoaded() before
	void shNginTBind() const;		// Assumes the texture is already loaded, use shNginTIsLoaded() before
	void shNginTTranslate(float x, float y);// Translates the texture so that different parts of texture
						// could be used in combination with the Ngin primitives
	void shNginTSetTransparent(unsigned char r, unsigned char g, unsigned char b,
				unsigned char a = 0, int m = NGIN_SMOOTH);
						// Sets alpha of all pixels who have the color rgb to a
						// If smooth, then pixels adjecent to the transparent area
						// will have (255+a)/2 transparency (half)
};

#endif
