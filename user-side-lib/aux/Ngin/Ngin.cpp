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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define				SHMACROS_LOG_PREFIX			"Ngin"

#include "shmacros.h"
#include "Ngin_opengl.h"
#include "Ngin_base.h"
#include "Ngin_light.h"

#define				PI					3.14159265358979323f
#define				PI_BY_2					6.28318530717958647f

#define				Z_NEAR					1
#define				Z_FAR					100100

bool				g_Ngin_initialized			= false;
static unsigned char		_Ngin_bgColor[4]			= {255, 255, 255, 255};
bool				g_Ngin_textureEnabled			= false;
static int			_Ngin_width,
				_Ngin_height;
static int			_Ngin_bottomLeftX			= 0;
static int			_Ngin_bottomLeftY			= 0;
static int			_Ngin_topRightX,
				_Ngin_topRightY;
const shNginTexture		*g_Ngin_currentTexture			= '\0';
static float			_Ngin_cameraPosition[3]			= {0, 0, 0};
static float			_Ngin_cameraTarget[3]			= {0, 0, -1};
static float			_Ngin_cameraUp[3]			= {0, 1, 0};
static int			_Ngin_fov				= 90;
static float			_Ngin_view_near_z			= Z_NEAR;
static float			_Ngin_view_far_z			= Z_FAR;
static float			_Ngin_view_ratio			= 1;

extern shNginLight		**g_Ngin_lightsDirty;			// From Ngin_light.cpp to delete[] when quitting

///////////////////////////////////////////////////
////////// Initialization and finilizing //////////
///////////////////////////////////////////////////
void shNginInitializeGL(int width, int height, int fov = 90)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glClearStencil(0);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat ambientLight[] = {0.2f, 0.2f, 0.2f, 1.0f};
	GLfloat diffuseLight[] = {0.0f, 0.0f, 0.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	shNginNewWindowSize(width, height, fov);
}

void shNginInitialize(int width, int height, int fov)
{
	if (g_Ngin_initialized)
		shNginQuit();
	if (!g_Ngin_lightsDirty)
	{
		int maxLights = shNginGetMaximumNumberOfLightsPossible();
		g_Ngin_lightsDirty = new shNginLight *[maxLights];
		memset(g_Ngin_lightsDirty, '\0', maxLights*sizeof(shNginLight *));
	}
	if (shmacros_log == '\0')
		shmacros_log = fopen("run.log", "w");
	++shmacros_log_users;
	shNginInitializeGL(width, height, fov);
	glDisable(GL_TEXTURE_2D);
	g_Ngin_initialized = true;
}

void shNginQuit()
{
	_Ngin_bgColor[0] = 255; _Ngin_bgColor[1] = 255; _Ngin_bgColor[2] = 255; _Ngin_bgColor[3] = 255;
	_Ngin_bottomLeftX = 0;
	_Ngin_bottomLeftY = 0;
	_Ngin_topRightX = _Ngin_width;
	_Ngin_topRightY = _Ngin_height;
	g_Ngin_initialized = false;
	g_Ngin_textureEnabled = false;
	g_Ngin_currentTexture = '\0';
	_Ngin_cameraPosition[0] = _Ngin_cameraPosition[1] = _Ngin_cameraPosition[2] = 0;
	_Ngin_cameraTarget[0] = _Ngin_cameraTarget[1] = 0; _Ngin_cameraTarget[2] = -1;
	_Ngin_cameraUp[0] = 0; _Ngin_cameraUp[1] = 1; _Ngin_cameraUp[2] = 0;
	_Ngin_fov = 90;
	_Ngin_view_near_z = Z_NEAR;
	_Ngin_view_far_z = Z_FAR;
	if (g_Ngin_lightsDirty)
	{
		delete[] g_Ngin_lightsDirty;
		g_Ngin_lightsDirty = '\0';
	}
	--shmacros_log_users;
	if (shmacros_log_users == 0 && shmacros_log)
		fclose(shmacros_log);
	g_Ngin_initialized = false;
}

///////////////////////////////////////////////////
//////////////// Window manipulation //////////////
///////////////////////////////////////////////////
void shNginNewWindowSize(int w, int h, int fov)
{
	_Ngin_width = w;
	_Ngin_height = h;
	glViewport(0, 0, w, h);
	shNginViewableArea(w, h, fov);
}

void shNginViewableArea(int w, int h, int fov)
{
	if (w > _Ngin_width) w = _Ngin_width;
	if (h > _Ngin_height) h = _Ngin_height;
	if (w <= 0) w = _Ngin_width;
	if (h <= 0) h = _Ngin_height;
	_Ngin_bottomLeftX = (_Ngin_width-w)/2;
	_Ngin_bottomLeftY = (_Ngin_height-h)/2;
	_Ngin_topRightX = _Ngin_width-1-(_Ngin_width-w)/2;
	_Ngin_topRightY = _Ngin_height-1-(_Ngin_height-h)/2;
	_Ngin_fov = fov;
	_Ngin_view_ratio = h/(float)w;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov*(float)_Ngin_height/w, _Ngin_width/(float)_Ngin_height, Z_NEAR, Z_FAR);
		// actually: fov*(float)_Ngin_width/w*_Ngin_height/_Ngin_width
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void shNginRefresh()
{
	glFlush();
	glLoadIdentity();
}

///////////////////////////////////////////////////
////////////////////// Buffers ////////////////////
///////////////////////////////////////////////////
void shNginScreenShot(unsigned char *img, int ox, int oy, int w, int h, int mode)
{
	if (w == 0 || h == 0)
	{
		ox = oy = 0;
		w = _Ngin_width;
		h = _Ngin_height;
	}
	glFinish();
	glReadBuffer(GL_FRONT);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glReadPixels(ox, oy, w, h, mode, GL_UNSIGNED_BYTE, img);
}

///////////////////////////////////////////////////
/////////////// Drawing manipulation //////////////
///////////////////////////////////////////////////
void shNginClear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glLoadIdentity();
	glEnable(GL_STENCIL_TEST);
	shNginDisableTexture();
	glColorMask(0, 0, 0, 0);
	glDepthMask(0);
	glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _Ngin_width, 0, _Ngin_height, -10000.0f, 10000.0f);
	glBegin(GL_QUADS);
		glVertex3f(_Ngin_bottomLeftX, _Ngin_bottomLeftY, 0);
		glVertex3f(_Ngin_topRightX, _Ngin_bottomLeftY, 0);
		glVertex3f(_Ngin_topRightX, _Ngin_topRightY, 0);
		glVertex3f(_Ngin_bottomLeftX, _Ngin_topRightY, 0);
	glEnd();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glColorMask(1, 1, 1, 1);
	glDepthMask(1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 1, 0xffffffff);
	glLoadIdentity();
	gluLookAt(_Ngin_cameraPosition[0], _Ngin_cameraPosition[1], _Ngin_cameraPosition[2],
		_Ngin_cameraTarget[0], _Ngin_cameraTarget[1], _Ngin_cameraTarget[2],
		_Ngin_cameraUp[0], _Ngin_cameraUp[1], _Ngin_cameraUp[2]);
	glColor4ub(255, 255, 255, 255);
}

void shNginEnableTexture()
{
	glEnable(GL_TEXTURE_2D);
	if (!g_Ngin_textureEnabled)
		g_Ngin_textureEnabled = true;
}

void shNginDisableTexture()
{
	glDisable(GL_TEXTURE_2D);
	if (g_Ngin_textureEnabled)
	{
		g_Ngin_textureEnabled = false;
		g_Ngin_currentTexture = '\0';
	}
}

void shNginSetCameraPosition(float x, float y, float z)
{
	_Ngin_cameraPosition[0] = x;
	_Ngin_cameraPosition[1] = y;
	_Ngin_cameraPosition[2] = z;
}

void shNginSetCameraPositionv(float p[3])
{
	_Ngin_cameraPosition[0] = p[0];
	_Ngin_cameraPosition[1] = p[1];
	_Ngin_cameraPosition[2] = p[2];
}

void shNginSetCameraTarget(float x, float y, float z)
{
	_Ngin_cameraTarget[0] = x;
	_Ngin_cameraTarget[1] = y;
	_Ngin_cameraTarget[2] = z;
}

void shNginSetCameraTargetv(float p[3])
{
	_Ngin_cameraTarget[0] = p[0];
	_Ngin_cameraTarget[1] = p[1];
	_Ngin_cameraTarget[2] = p[2];
}

void shNginSetCameraUp(float x, float y, float z)
{
	_Ngin_cameraUp[0] = x;
	_Ngin_cameraUp[1] = y;
	_Ngin_cameraUp[2] = z;
}

void shNginSetCameraUpv(float p[3])
{
	_Ngin_cameraUp[0] = p[0];
	_Ngin_cameraUp[1] = p[1];
	_Ngin_cameraUp[2] = p[2];
}

///////////////////////////////////////////////////
///////////////////// Frustum /////////////////////
///////////////////////////////////////////////////
void shNginSetNearZ(float z)
{
	if (z < _Ngin_view_far_z)
		_Ngin_view_near_z = z;
}

void shNginSetFarZ(float z)
{
	if (z > _Ngin_view_near_z)
		_Ngin_view_far_z = z;
}

void shNginGetViewFrustum(shNginFrustum *frustum)
{
	float dir[3] = {_Ngin_cameraTarget[0]-_Ngin_cameraPosition[0], _Ngin_cameraTarget[1]-_Ngin_cameraPosition[1], _Ngin_cameraTarget[2]-_Ngin_cameraPosition[2]};
	frustum->shNginFCreateFrustum(_Ngin_cameraPosition, dir, _Ngin_cameraUp, _Ngin_view_near_z, _Ngin_view_far_z, _Ngin_fov, _Ngin_view_ratio);
}

///////////////////////////////////////////////////
//////////////////// Utilities ////////////////////
///////////////////////////////////////////////////
unsigned char *shNginConvertImage(const unsigned char *image, unsigned int width, unsigned int height,
								int fromMode, int toMode)
{
	// Each image row is aligned to 4 bytes, so be careful!
	unsigned char *newArray;
	int fromConvert[4] = {0, 1, 2, 3};
	int toConvert[4] = {0, 1, 2, 3};
	int f = 0, t = 0;
	switch (fromMode)
	{
		case NGIN_BGR:
			fromConvert[0] = 2;
			fromConvert[2] = 0;
		case NGIN_RGB:
			f = 3;
			break;
		case NGIN_RGBA:
			f = 4;
			break;
		default:
			LOG("shNginConvertImage: undefined type of image to convert");
	}
	switch (toMode)
	{
		case NGIN_BGR:
			toConvert[0] = 2;
			toConvert[2] = 0;
		case NGIN_RGB:
			t = 3;
			break;
		case NGIN_RGBA:
			t = 4;
			break;
		default:
			LOG("shNginConvertImage: undefined type for image to convert to");
	}
	int widthToUseFrom = width*f;
	if (widthToUseFrom&0x03)
		widthToUseFrom += 4-(widthToUseFrom&0x03);
	int widthToUseTo = width*t;
	if (widthToUseTo&0x03)
		widthToUseTo += 4-(widthToUseTo&0x03);
	newArray = new unsigned char[widthToUseTo*height];
	int m = MIN(f, t);
	for (unsigned int r = 0; r < height; ++r)
		for (unsigned int c = 0; c < width; ++c)
		{
			int fromIndex = r*widthToUseFrom+c*f;
			int toIndex = r*widthToUseTo+c*t;
			for (int i = 0; i < m; ++i)
				newArray[toIndex+toConvert[i]] = image[fromIndex+fromConvert[i]];
			for (int i = m; i < t; ++i)	// If t<=f, then ignored, if t>f the rest is filled with 255
				newArray[toIndex+i] = 255;
		}
	return newArray;
}

void shNginrgb2hsl(unsigned char r, unsigned char g, unsigned char b,
		unsigned char *h, unsigned char *s, unsigned char *l)
{
	float fr = (float)r/255, fg = (float)g/255, fb = (float)b/255;
	float maxColor = MAX(fr, fg);
	maxColor = MAX(maxColor, fb);
	float minColor = MIN(fr, fg);
	minColor = MIN(minColor, fb);
	float L = (maxColor+minColor)/2;
	float H, S;
	if (maxColor == minColor)
	{
		H = 0;
		S = 0;
	}
	else
	{
		if (L < 0.5f)
			S = (maxColor-minColor)/(maxColor+minColor);
		else
			S = (maxColor-minColor)/(2.0f-maxColor-minColor);
		if (fr == maxColor)
			H = (fg-fb)/(maxColor-minColor);
		else if (fg == maxColor)
			H = 2.0f+(fb-fr)/(maxColor-minColor);
		else
			H = 4.0f+(fr-fg)/(maxColor-minColor);
	}
	if (H < 0)
		H += 6;
	H = H*255/6;
	L *= 255;
	S *= 255;
	if (H >= 255.0f)
		*h = 255;
	else
		*h = (unsigned char)H;
	if (S >= 255.0f)
		*s = 255;
	else
		*s = (unsigned char)S;
	if (L >= 255.0f)
		*l = 255;
	else
		*l = (unsigned char)L;
}

void shNginhsl2rgb(unsigned char h, unsigned char s, unsigned char l,
		unsigned char *r, unsigned char *g, unsigned char *b)
{
	if (s == 0)
	{
		*r = *g = *b = l;
		return;
	}
	float fh = (float)h/255, fs = (float)s/255, fl = (float)l/255;
	float temp1, temp2;
	if (fl < 0.5)
		temp2 = fl*(1.0f+fs);
	else
		temp2 = fl+fs-fl*fs;
	temp1 = 2.0f*fl-temp2;
	float temp3[3] = {fh+1.0f/3.0f, fh, fh-1.0f/3.0f};
	if (temp3[0] > 1.0f)
		temp3[0] -= 1.0f;
	if (temp3[2] < 0)
		temp3[2] += 1.0f;
	float RGB[3];
	for (int i = 0; i < 3; ++i)
	{
		if (temp3[i]*6.0f < 1.0f)
			RGB[i] = temp1+(temp2-temp1)*6.0f*temp3[i];
		else if (temp3[i]*2.0f < 1.0f)
			RGB[i] = temp2;
		else if (temp3[i]*3.0f < 2.0f)
			RGB[i] = temp1+(temp2-temp1)*((2.0f/3.0f)-temp3[i])*6.0f;
		else
			RGB[i] = temp1;
		RGB[i] *= 255;
		if (RGB[i] > 255)
			RGB[i] = 255;
	}
	*r = (unsigned char)RGB[0];
	*g = (unsigned char)RGB[1];
	*b = (unsigned char)RGB[2];
}
