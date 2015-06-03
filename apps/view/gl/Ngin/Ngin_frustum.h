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

#ifndef NGIN_FRUSTUM_H_BY_CYCLOPS
#define NGIN_FRUSTUM_H_BY_CYCLOPS

#define				NGIN_FRUSTUM_FRONT			0
#define				NGIN_FRUSTUM_BOTTOM			1
#define				NGIN_FRUSTUM_LEFT			2
#define				NGIN_FRUSTUM_BACK			3
#define				NGIN_FRUSTUM_TOP			4
#define				NGIN_FRUSTUM_RIGHT			5

#define				NGIN_FRUSTUM_FRONT_BOTTOM_LEFT		0
#define				NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT		1
#define				NGIN_FRUSTUM_FRONT_TOP_RIGHT		2
#define				NGIN_FRUSTUM_FRONT_TOP_LEFT		3
#define				NGIN_FRUSTUM_BACK_BOTTOM_LEFT		4
#define				NGIN_FRUSTUM_BACK_BOTTOM_RIGHT		5
#define				NGIN_FRUSTUM_BACK_TOP_RIGHT		6
#define				NGIN_FRUSTUM_BACK_TOP_LEFT		7

class shNginFrustum
{
public:
	float vertices[8][3];										// vertices are of the following order: front-bottom-left, front-bottom-right, front-top-right, front-top-left,
													//					 back-bottom-left,  back-bottom-right,  back-top-right,  back-top-left
													//					 as defined above
	float normals[6][3];										// normals are for the following planes: front, bottom, left, back, top, right as defined above
													// normals must point OUTSIDE the frustum
	shNginFrustum();
	shNginFrustum(float *frontBottomLeft, float *backTopRight, float *n1, float *n2, float *n3, float *n4, float *n5, float *n6);
													// normals given in the following order: front, bottom, left, back, top, right
	shNginFrustum(float *frontBottomLeft, float *frontBottomRight, float *frontTopRight, float depth, float fov_x);		// fov_x is the field of view in x direction expressed in degrees
	shNginFrustum(float *eye, float *direction, float *up, float znear, float zfar, float fov_x, float ratio);		// fov_x is the field of view in x direction expressed in degrees,
																// ratio is the aspect ratio of y to x
	shNginFrustum(const shNginFrustum &RHS);
	~shNginFrustum();
	void shNginFCreateFrustum(float *frontBottomLeft, float *backTopRight, float *n1, float *n2, float *n3, float *n4, float *n5, float *n6);
													// normals given in the following order: front, bottom, left, back, top, right
	void shNginFCreateFrustum(float *frontBottomLeft, float *frontBottomRight, float *frontTopRight, float depth, float fov_x);	// fov_x is the field of view in x direction expressed in degrees
	void shNginFCreateFrustum(float *eye, float *direction, float *up, float znear, float zfar, float fov_x, float ratio);		// fov_x is the field of view in x direction expressed in degrees
																	// ratio is the aspect ratio of y to x
	bool shNginFPointInside(float *point);								// check to see if the point lies inside the frustum
	bool shNginFPortalCrosses(float *bottom_left, float *bottom_right, float *top_right);		// check to see if the identified rectangle crosses the frustum (taking into account the direction of the portal)
	void shNginFClipFrustumWithPortal(float *bottom_left, float *bottom_right, float *top_right, shNginFrustum *result);	// portal is basically a rectangle. Make sure to check the visibility of the rectangle with
													// shNginFPortalCrosses. This functions assumes the portal IS visibile.
	bool shNginFObjectBoundaryCrosses(float *c, float r);						// c and r represent a sphere that is the boundary of an object
	shNginFrustum &operator =(const shNginFrustum &RHS);
};

#endif
