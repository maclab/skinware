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

#ifndef NGIN_BASE_H_BY_CYCLOPS
#define NGIN_BASE_H_BY_CYCLOPS

#include "Ngin_texture.h"
#include "Ngin_frustum.h"

// Initialization and finilizing
// value of fov is the field of view in the x direction.
void shNginInitialize(int width, int height, int fov = 90);
void shNginQuit();

// Window manipulation
void shNginNewWindowSize(int w, int h, int fov = 90);
// shNginViewableArea uses stencil buffer to block parts of the screen from being drawn. The stencil buffer is cleared
// with 1 and the masked area with 0. The viewable area is centered in the window. All position values, including mouse
// positions, are automatically converted to the viewable area. Therefore, the application can be written as if the
// viewable area is the whole window itself.
// This function is mainly used for full screen applications that want the same aspect ratio with different monitor
// sizes
void shNginViewableArea(int w, int h, int fov = 90);
void shNginRefresh();

// Buffers
// Store a window of the frame buffer enclosed in (ox,oy) and (ox+w,oy+h) in img with specified mode
// If no parameter specified, captures the whole screen
void shNginScreenShot(unsigned char *img, int ox = 0, int oy = 0, int w = 0, int h = 0, int mode = NGIN_BGR);

// Drawing manipulation
void shNginClear();
void shNginEnableTexture();
void shNginDisableTexture();
void shNginSetCameraPosition(float x, float y, float z);
void shNginSetCameraPositionv(float p[3]);
void shNginSetCameraTarget(float x, float y, float z);
void shNginSetCameraTargetv(float p[3]);
void shNginSetCameraUp(float x, float y, float z);
void shNginSetCameraUpv(float p[3]);

// Frustum
void shNginSetNearZ(float z);
void shNginSetFarZ(float z);
void shNginGetViewFrustum(shNginFrustum *frustum);
// TODO: Do something about the portals. Best way is to have the portals defined in Ngin3d and the Ngin would simply apply the portal, which is basically updating the stencil buffer

// Utilities
unsigned char *shNginConvertImage(const unsigned char *img, unsigned int width, unsigned int height,
		int fromMode = NGIN_BGR, int toMode = NGIN_RGBA);
void shNginrgb2hsl(unsigned char r, unsigned char g, unsigned char b,
		unsigned char *h, unsigned char *s, unsigned char *l);
void shNginhsl2rgb(unsigned char h, unsigned char s, unsigned char l,
		unsigned char *r, unsigned char *g, unsigned char *b);

#endif
