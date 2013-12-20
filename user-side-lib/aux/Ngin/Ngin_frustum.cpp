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
#include <string.h>
#include "Ngin_frustum.h"

#define				SHMACROS_LOG_PREFIX			"Ngin"

#include "shmacros.h"

#define				PI					3.14159265358979323f
#define				PI_BY_2					6.28318530717958647f
#define				EPSILON					1e-10

shNginFrustum::shNginFrustum()
{
	for (int i = 0; i < 8; ++i)
		vertices[i][0] = vertices[i][1] = vertices[i][2] = 0;
	for (int i = 0; i < 6; ++i)
	{
		normals[i][0] = 0;
		normals[i][1] = 0;
		normals[i][2] = 1;
	}
}

shNginFrustum::shNginFrustum(float *frontBottomLeft, float *backTopRight, float *n1, float *n2, float *n3, float *n4, float *n5, float *n6)
{
	shNginFCreateFrustum(frontBottomLeft, backTopRight, n1, n2, n3, n4, n5, n6);
}

shNginFrustum::shNginFrustum(float *frontBottomLeft, float *frontBottomRight, float *frontTopRight, float depth, float fov_x)
{
	shNginFCreateFrustum(frontBottomLeft, frontBottomRight, frontTopRight, depth, fov_x);
}

shNginFrustum::shNginFrustum(float *eye, float *direction, float *up, float znear, float zfar, float fov_x, float ratio)
{
	shNginFCreateFrustum(eye, direction, up, znear, zfar, fov_x, ratio);
}

shNginFrustum::shNginFrustum(const shNginFrustum &RHS)
{
	*this = RHS;
}

shNginFrustum::~shNginFrustum()
{
}

void shNginFrustum::shNginFCreateFrustum(float *frontBottomLeft, float *backTopRight, float *n1, float *n2, float *n3, float *n4, float *n5, float *n6)
{
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0] = frontBottomLeft[0];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1] = frontBottomLeft[1];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2] = frontBottomLeft[2];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0] = backTopRight[0];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1] = backTopRight[1];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2] = backTopRight[2];
	normals[NGIN_FRUSTUM_FRONT][0] = n1[0]; normals[NGIN_FRUSTUM_FRONT][1] = n1[1]; normals[NGIN_FRUSTUM_FRONT][2] = n1[2];
	normals[NGIN_FRUSTUM_BOTTOM][0] = n2[0]; normals[NGIN_FRUSTUM_BOTTOM][1] = n2[1]; normals[NGIN_FRUSTUM_BOTTOM][2] = n2[2];
	normals[NGIN_FRUSTUM_LEFT][0] = n3[0]; normals[NGIN_FRUSTUM_LEFT][1] = n3[1]; normals[NGIN_FRUSTUM_LEFT][2] = n3[2];
	normals[NGIN_FRUSTUM_BACK][0] = n4[0]; normals[NGIN_FRUSTUM_BACK][1] = n4[1]; normals[NGIN_FRUSTUM_BACK][2] = n4[2];
	normals[NGIN_FRUSTUM_TOP][0] = n5[0]; normals[NGIN_FRUSTUM_TOP][1] = n5[1]; normals[NGIN_FRUSTUM_TOP][2] = n5[2];
	normals[NGIN_FRUSTUM_RIGHT][0] = n6[0]; normals[NGIN_FRUSTUM_RIGHT][1] = n6[1]; normals[NGIN_FRUSTUM_RIGHT][2] = n6[2];
	for (int i = 0; i < 6; ++i)
	{
		float norm = NORM(normals[i]);
		if (norm < EPSILON)
		{
			normals[i][0] = 0;
			normals[i][1] = 0;
			normals[i][2] = 1;
		}
		else
		{
			normals[i][0] /= norm;
			normals[i][1] /= norm;
			normals[i][2] /= norm;
		}
	}
	// Now, find the rest of the points. They are the points were three planes of different sides collide.
	float RHS[6] = {DOT(normals[NGIN_FRUSTUM_FRONT], vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT]),
			DOT(normals[NGIN_FRUSTUM_BOTTOM], vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT]),
			DOT(normals[NGIN_FRUSTUM_LEFT], vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT]),
			DOT(normals[NGIN_FRUSTUM_BACK], vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT]),
			DOT(normals[NGIN_FRUSTUM_TOP], vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT]),
			DOT(normals[NGIN_FRUSTUM_RIGHT], vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT])};
	float det = 0;
#define DET(x11,x12,x13,x21,x22,x23,x31,x32,x33) ((x11)*(x22)*(x33)+(x12)*(x23)*(x31)+(x13)*(x21)*(x32)-(x11)*(x23)*(x32)-(x12)*(x21)*(x33)-(x13)*(x22)*(x31))
#define SOLVE_VERTEX(side1, side2, side3) \
	det = DET(normals[NGIN_FRUSTUM_##side1][0], normals[NGIN_FRUSTUM_##side1][1], normals[NGIN_FRUSTUM_##side1][2],\
		normals[NGIN_FRUSTUM_##side2][0], normals[NGIN_FRUSTUM_##side2][1], normals[NGIN_FRUSTUM_##side2][2],\
		normals[NGIN_FRUSTUM_##side3][0], normals[NGIN_FRUSTUM_##side3][1], normals[NGIN_FRUSTUM_##side3][2]);\
	if (det < EPSILON) { *this = shNginFrustum(); return; }\
	vertices[NGIN_FRUSTUM_##side1##_##side2##_##side3][0] = DET(RHS[NGIN_FRUSTUM_##side1], normals[NGIN_FRUSTUM_##side1][1], normals[NGIN_FRUSTUM_##side1][2],\
								RHS[NGIN_FRUSTUM_##side2], normals[NGIN_FRUSTUM_##side2][1], normals[NGIN_FRUSTUM_##side2][2],\
								RHS[NGIN_FRUSTUM_##side3], normals[NGIN_FRUSTUM_##side3][1], normals[NGIN_FRUSTUM_##side3][2])/det;\
	vertices[NGIN_FRUSTUM_##side1##_##side2##_##side3][1] = DET(normals[NGIN_FRUSTUM_##side1][0], RHS[NGIN_FRUSTUM_##side1], normals[NGIN_FRUSTUM_##side1][2],\
								normals[NGIN_FRUSTUM_##side2][0], RHS[NGIN_FRUSTUM_##side2], normals[NGIN_FRUSTUM_##side2][2],\
								normals[NGIN_FRUSTUM_##side3][0], RHS[NGIN_FRUSTUM_##side3], normals[NGIN_FRUSTUM_##side3][2])/det;\
	vertices[NGIN_FRUSTUM_##side1##_##side2##_##side3][2] = DET(normals[NGIN_FRUSTUM_##side1][0], normals[NGIN_FRUSTUM_##side1][1], RHS[NGIN_FRUSTUM_##side1],\
								normals[NGIN_FRUSTUM_##side2][0], normals[NGIN_FRUSTUM_##side2][1], RHS[NGIN_FRUSTUM_##side2],\
								normals[NGIN_FRUSTUM_##side3][0], normals[NGIN_FRUSTUM_##side3][1], RHS[NGIN_FRUSTUM_##side3])/det
	SOLVE_VERTEX(FRONT, BOTTOM, RIGHT);
	SOLVE_VERTEX(FRONT, TOP, RIGHT);
	SOLVE_VERTEX(FRONT, TOP, LEFT);
	SOLVE_VERTEX(BACK, BOTTOM, LEFT);
	SOLVE_VERTEX(BACK, BOTTOM, RIGHT);
	SOLVE_VERTEX(BACK, TOP, LEFT);
}

void shNginFrustum::shNginFCreateFrustum(float *frontBottomLeft, float *frontBottomRight, float *frontTopRight, float depth, float fov_x)
{
	if (depth < EPSILON || fov_x < EPSILON || fov_x > 180-EPSILON)
	{
		*this = shNginFrustum();
		return;
	}
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0] = frontBottomLeft[0];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1] = frontBottomLeft[1];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2] = frontBottomLeft[2];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0] = frontBottomRight[0];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1] = frontBottomRight[1];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2] = frontBottomRight[2];
	vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0] = frontTopRight[0];
	vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1] = frontTopRight[1];
	vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2] = frontTopRight[2];
	float dir1[3] = {vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0],
			vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1],
			vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]};
	float dir2[3] = {vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0],
			vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1],
			vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2]};
	vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][0] = vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0]+dir2[0];
	vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][1] = vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1]+dir2[1];
	vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][2] = vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]+dir2[2];
	float direction[3] = {CROSS(dir2, dir1)};
	float norm = NORM(direction);
	if (norm < EPSILON)
	{
		*this = shNginFrustum();
		return;
	}
	direction[0] /= norm;
	direction[1] /= norm;
	direction[2] /= norm;
	float frustumEyeDistance = NORM(dir1)/2/tan(fov_x*PI/360.0f);		// fov_x/2 * PI/180
	float centerFront[3] = {(vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0]+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0])/2.0f,
				(vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1]+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1])/2.0f,
				(vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2])/2.0f};
	float eyePosition[3] = {centerFront[0]-direction[0]*frustumEyeDistance,
				centerFront[1]-direction[1]*frustumEyeDistance,
				centerFront[2]-direction[2]*frustumEyeDistance};
	float ratio = depth/frustumEyeDistance;
	float dirToTopRight[3] = {vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0]-eyePosition[0],
				vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1]-eyePosition[1],
				vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2]-eyePosition[2]};
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0] = dirToTopRight[0]*ratio+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1] = dirToTopRight[1]*ratio+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2] = dirToTopRight[2]*ratio+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2];
	float dirToTopLeft[3] = {vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][0]-eyePosition[0],
				vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][1]-eyePosition[1],
				vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][2]-eyePosition[2]};
	vertices[NGIN_FRUSTUM_BACK_TOP_LEFT][0] = dirToTopLeft[0]*ratio+vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][0];
	vertices[NGIN_FRUSTUM_BACK_TOP_LEFT][1] = dirToTopLeft[1]*ratio+vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][1];
	vertices[NGIN_FRUSTUM_BACK_TOP_LEFT][2] = dirToTopLeft[2]*ratio+vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][2];
	float dirToBottomRight[3] = {vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0]-eyePosition[0],
				vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1]-eyePosition[1],
				vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2]-eyePosition[2]};
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_RIGHT][0] = dirToBottomRight[0]*ratio+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_RIGHT][1] = dirToBottomRight[1]*ratio+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_RIGHT][2] = dirToBottomRight[2]*ratio+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2];
	float dirToBottomLeft[3] = {vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0]-eyePosition[0],
				vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1]-eyePosition[1],
				vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]-eyePosition[2]};
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][0] = dirToBottomLeft[0]*ratio+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][1] = dirToBottomLeft[1]*ratio+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][2] = dirToBottomLeft[2]*ratio+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2];

	normals[NGIN_FRUSTUM_FRONT][0] = -direction[0]; normals[NGIN_FRUSTUM_FRONT][1] = -direction[1]; normals[NGIN_FRUSTUM_FRONT][2] = -direction[2];
	normals[NGIN_FRUSTUM_BACK][0] = direction[0]; normals[NGIN_FRUSTUM_BACK][1] = direction[1]; normals[NGIN_FRUSTUM_BACK][2] = direction[2];
	normals[NGIN_FRUSTUM_RIGHT][0] = CROSS_X(dirToTopRight, dir2);
	normals[NGIN_FRUSTUM_RIGHT][1] = CROSS_Y(dirToTopRight, dir2);
	normals[NGIN_FRUSTUM_RIGHT][2] = CROSS_Z(dirToTopRight, dir2);
	normals[NGIN_FRUSTUM_LEFT][0] = CROSS_X(dir2, dirToBottomLeft);
	normals[NGIN_FRUSTUM_LEFT][1] = CROSS_Y(dir2, dirToBottomLeft);
	normals[NGIN_FRUSTUM_LEFT][2] = CROSS_Z(dir2, dirToBottomLeft);
	normals[NGIN_FRUSTUM_TOP][0] = CROSS_X(dir1, dirToTopRight);
	normals[NGIN_FRUSTUM_TOP][1] = CROSS_Y(dir1, dirToTopRight);
	normals[NGIN_FRUSTUM_TOP][2] = CROSS_Z(dir1, dirToTopRight);
	normals[NGIN_FRUSTUM_BOTTOM][0] = CROSS_X(dirToBottomLeft, dir1);
	normals[NGIN_FRUSTUM_BOTTOM][1] = CROSS_Y(dirToBottomLeft, dir1);
	normals[NGIN_FRUSTUM_BOTTOM][2] = CROSS_Z(dirToBottomLeft, dir1);
	norm = NORM(normals[NGIN_FRUSTUM_RIGHT]);				// none of the norms should be zero since depth is not, fov_x is proper and direction has non zero norm (and therefore dir1 and dir2)
	normals[NGIN_FRUSTUM_RIGHT][0] /= norm;
	normals[NGIN_FRUSTUM_RIGHT][1] /= norm;
	normals[NGIN_FRUSTUM_RIGHT][2] /= norm;
	norm = NORM(normals[NGIN_FRUSTUM_LEFT]);
	normals[NGIN_FRUSTUM_LEFT][0] /= norm;
	normals[NGIN_FRUSTUM_LEFT][1] /= norm;
	normals[NGIN_FRUSTUM_LEFT][2] /= norm;
	norm = NORM(normals[NGIN_FRUSTUM_TOP]);
	normals[NGIN_FRUSTUM_TOP][0] /= norm;
	normals[NGIN_FRUSTUM_TOP][1] /= norm;
	normals[NGIN_FRUSTUM_TOP][2] /= norm;
	norm = NORM(normals[NGIN_FRUSTUM_BOTTOM]);
	normals[NGIN_FRUSTUM_BOTTOM][0] /= norm;
	normals[NGIN_FRUSTUM_BOTTOM][1] /= norm;
	normals[NGIN_FRUSTUM_BOTTOM][2] /= norm;
}

void shNginFrustum::shNginFCreateFrustum(float *eye, float *direction, float *up, float znear, float zfar, float fov_x, float ratio)
{
	if (znear < EPSILON || zfar-znear < EPSILON || fov_x < EPSILON || fov_x > 180-EPSILON)
	{
		*this = shNginFrustum();
		return;
	}
	float norm = NORM(direction);
	if (norm < EPSILON)
	{
		*this = shNginFrustum();
		return;
	}
	direction[0] /= norm;
	direction[1] /= norm;
	direction[2] /= norm;
	norm = NORM(up);
	if (norm < EPSILON)
	{
		*this = shNginFrustum();
		return;
	}
	up[0] /= norm;
	up[1] /= norm;
	up[2] /= norm;
	float right[3] = {CROSS(direction, up)};
	norm = NORM(right);
	if (norm < EPSILON)				// that is, if direction and up were on the same direction
	{
		*this = shNginFrustum();
		return;
	}
	float real_up[3] = {CROSS(right, direction)};	// because sometimes, up is simply considered (0, 1, 0) instead of real up in game engines
	float alphaover2 = fov_x*PI/360;		// fov_x/2 * PI/180
	float dx = tan(alphaover2);
	float dy = dx*ratio;
	float dxnear = dx*znear;
	float dynear = dy*znear;
	float dxfar = dx*zfar;
	float dyfar = dy*zfar;
	float rightnear[3] = {right[0]*dxnear, right[1]*dxnear, right[2]*dxnear};
	float real_upnear[3] = {real_up[0]*dynear, real_up[1]*dynear, real_up[2]*dynear};
	float rightfar[3] = {right[0]*dxfar, right[1]*dxfar, right[2]*dxfar};
	float real_upfar[3] = {real_up[0]*dyfar, real_up[1]*dyfar, real_up[2]*dyfar};
	float directionnear[3] = {eye[0]+znear*direction[0], eye[1]+znear*direction[1], eye[2]+znear*direction[2]};
	float directionfar[3] = {eye[0]+zfar*direction[0], eye[1]+zfar*direction[1], eye[2]+zfar*direction[2]};
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0] = directionnear[0]-real_upnear[0]-rightnear[0];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1] = directionnear[1]-real_upnear[1]-rightnear[1];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2] = directionnear[2]-real_upnear[2]-rightnear[2];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0] = directionnear[0]-real_upnear[0]+rightnear[0];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1] = directionnear[1]-real_upnear[1]+rightnear[1];
	vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2] = directionnear[2]-real_upnear[2]+rightnear[2];
	vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0] = directionnear[0]+real_upnear[0]+rightnear[0];
	vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1] = directionnear[1]+real_upnear[1]+rightnear[1];
	vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2] = directionnear[2]+real_upnear[2]+rightnear[2];
	vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][0] = directionnear[0]+real_upnear[0]-rightnear[0];
	vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][1] = directionnear[1]+real_upnear[1]-rightnear[1];
	vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][2] = directionnear[2]+real_upnear[2]-rightnear[2];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][0] = directionfar[0]-real_upfar[0]-rightfar[0];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][1] = directionfar[1]-real_upfar[1]-rightfar[1];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][2] = directionfar[2]-real_upfar[2]-rightfar[2];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_RIGHT][0] = directionfar[0]-real_upfar[0]+rightfar[0];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_RIGHT][1] = directionfar[1]-real_upfar[1]+rightfar[1];
	vertices[NGIN_FRUSTUM_BACK_BOTTOM_RIGHT][2] = directionfar[2]-real_upfar[2]+rightfar[2];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0] = directionfar[0]+real_upfar[0]+rightfar[0];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1] = directionfar[1]+real_upfar[1]+rightfar[1];
	vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2] = directionfar[2]+real_upfar[2]+rightfar[2];
	vertices[NGIN_FRUSTUM_BACK_TOP_LEFT][0] = directionfar[0]+real_upfar[0]-rightfar[0];
	vertices[NGIN_FRUSTUM_BACK_TOP_LEFT][1] = directionfar[1]+real_upfar[1]-rightfar[1];
	vertices[NGIN_FRUSTUM_BACK_TOP_LEFT][2] = directionfar[2]+real_upfar[2]-rightfar[2];
	normals[NGIN_FRUSTUM_FRONT][0] = -direction[0]; normals[NGIN_FRUSTUM_FRONT][1] = -direction[1]; normals[NGIN_FRUSTUM_FRONT][2] = -direction[2];
	normals[NGIN_FRUSTUM_BACK][0] = direction[0]; normals[NGIN_FRUSTUM_BACK][1] = direction[1]; normals[NGIN_FRUSTUM_BACK][2] = direction[2];
	float dirToTopRight[3] = {vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0]-vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0],
				vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1]-vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1],
				vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2]-vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2]};
	float dirToBottomLeft[3] = {vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0],
				vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1],
				vertices[NGIN_FRUSTUM_BACK_BOTTOM_LEFT][2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]};
	normals[NGIN_FRUSTUM_RIGHT][0] = CROSS_X(dirToTopRight, real_up);
	normals[NGIN_FRUSTUM_RIGHT][1] = CROSS_Y(dirToTopRight, real_up);
	normals[NGIN_FRUSTUM_RIGHT][2] = CROSS_Z(dirToTopRight, real_up);
	normals[NGIN_FRUSTUM_LEFT][0] = CROSS_X(real_up, dirToBottomLeft);
	normals[NGIN_FRUSTUM_LEFT][1] = CROSS_Y(real_up, dirToBottomLeft);
	normals[NGIN_FRUSTUM_LEFT][2] = CROSS_Z(real_up, dirToBottomLeft);
	normals[NGIN_FRUSTUM_TOP][0] = CROSS_X(right, dirToTopRight);
	normals[NGIN_FRUSTUM_TOP][1] = CROSS_Y(right, dirToTopRight);
	normals[NGIN_FRUSTUM_TOP][2] = CROSS_Z(right, dirToTopRight);
	normals[NGIN_FRUSTUM_BOTTOM][0] = CROSS_X(dirToBottomLeft, right);
	normals[NGIN_FRUSTUM_BOTTOM][1] = CROSS_Y(dirToBottomLeft, right);
	normals[NGIN_FRUSTUM_BOTTOM][2] = CROSS_Z(dirToBottomLeft, right);
	norm = NORM(normals[NGIN_FRUSTUM_RIGHT]);				// none of the norms should be zero
	normals[NGIN_FRUSTUM_RIGHT][0] /= norm;
	normals[NGIN_FRUSTUM_RIGHT][1] /= norm;
	normals[NGIN_FRUSTUM_RIGHT][2] /= norm;
	norm = NORM(normals[NGIN_FRUSTUM_LEFT]);
	normals[NGIN_FRUSTUM_LEFT][0] /= norm;
	normals[NGIN_FRUSTUM_LEFT][1] /= norm;
	normals[NGIN_FRUSTUM_LEFT][2] /= norm;
	norm = NORM(normals[NGIN_FRUSTUM_TOP]);
	normals[NGIN_FRUSTUM_TOP][0] /= norm;
	normals[NGIN_FRUSTUM_TOP][1] /= norm;
	normals[NGIN_FRUSTUM_TOP][2] /= norm;
	norm = NORM(normals[NGIN_FRUSTUM_BOTTOM]);
	normals[NGIN_FRUSTUM_BOTTOM][0] /= norm;
	normals[NGIN_FRUSTUM_BOTTOM][1] /= norm;
	normals[NGIN_FRUSTUM_BOTTOM][2] /= norm;
}

bool shNginFrustum::shNginFPointInside(float *point)
{
	float vec[3] = {point[0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0],
			point[1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1],
			point[2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]};
	for (int i = 0; i < 3; ++i)
		if (DOT(normals[i], vec) >= 0)
			return false;
	vec[0] = point[0]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0];
	vec[1] = point[1]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1];
	vec[2] = point[2]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2];
	for (int i = 3; i < 6; ++i)
		if (DOT(normals[i], vec) >= 0)
			return false;
	return true;
}

bool shNginFrustum::shNginFPortalCrosses(float *bottom_left, float *bottom_right, float *top_right)
{
	float top_left[3] = {bottom_left[0]+top_right[0]-bottom_right[0],
			bottom_left[1]+top_right[1]-bottom_right[1],
			bottom_left[2]+top_right[2]-bottom_right[2]};
	// Check to see if the eye is on the right side of the portal to see through. That is, if (portal-eye).portal_direction < 0 then return false
	float dir1[3] = {bottom_right[0]-bottom_left[0], bottom_right[1]-bottom_left[1], bottom_right[2]-bottom_left[2]};
	float dir2[3] = {top_right[0]-bottom_right[0], top_right[1]-bottom_right[1], top_right[2]-bottom_right[2]};
	float direction[3] = {CROSS(dir2, dir1)};
	float eye[3] = {(vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0]+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][0]+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][0]+vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][0])/4,
			(vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1]+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][1]+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][1]+vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][1])/4,
			(vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]+vertices[NGIN_FRUSTUM_FRONT_BOTTOM_RIGHT][2]+vertices[NGIN_FRUSTUM_FRONT_TOP_RIGHT][2]+vertices[NGIN_FRUSTUM_FRONT_TOP_LEFT][2])/4};
	float eyeToPortal[3] = {bottom_left[0]-eye[0], bottom_left[1]-eye[1], bottom_left[2]-eye[2]};
	if (DOT(direction, eyeToPortal) < 0)
		return false;
	// Check each of the rectangle's points to see if they lie on one side of a frustum plane. If they do, then return false
	float vec[4][3] = {{bottom_left[0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0], bottom_left[1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1], bottom_left[2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]},
			{bottom_right[0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0], bottom_right[1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1], bottom_right[2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]},
			{top_left[0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0], top_left[1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1], top_left[2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]},
			{top_right[0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0], top_right[1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1], top_right[2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]}};
	for (int i = 0; i < 3; ++i)
	{
		bool allOutside = true;
		for (int j = 0; j < 4; ++j)
			if (DOT(normals[i], vec[j]) < 0)
			{
				allOutside = false;
				break;
			}
		if (allOutside)
			return false;
	}
	float vec2[4][3] = {{bottom_left[0]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0], bottom_left[1]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1], bottom_left[2]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2]},
			{bottom_right[0]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0], bottom_right[1]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1], bottom_right[2]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2]},
			{top_left[0]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0], top_left[1]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1], top_left[2]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2]},
			{top_right[0]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0], top_right[1]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1], top_right[2]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2]}};
	for (int i = 3; i < 6; ++i)
	{
		bool allOutside = true;
		for (int j = 0; j < 4; ++j)
			if (DOT(normals[i], vec2[j]) < 0)
			{
				allOutside = false;
				break;
			}
		if (allOutside)
			return false;
	}
	// Check each of the frustum corners against the rectangle. If they all lie on the same side, then again return false
	int onSides = 0;
	for (int i = 0; i < 8; ++i)
	{
		float vec[3] = {vertices[i][0]-bottom_left[0], vertices[i][1]-bottom_left[1], vertices[i][2]-bottom_left[2]};
		if (DOT(vec, direction) > 0)
			++onSides;
		else
			--onSides;
	}
	if (onSides == 8 || onSides == -8)
		return false;
	// TODO: Check to see if there is any other condition returning false
	return true;
}

void shNginFrustum::shNginFClipFrustumWithPortal(float *bottom_left, float *bottom_right, float *top_right, shNginFrustum *result)
{
}

bool shNginFrustum::shNginFObjectBoundaryCrosses(float *c, float r)
{
	float vec[3] = {c[0]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][0],
			c[1]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][1],
			c[2]-vertices[NGIN_FRUSTUM_FRONT_BOTTOM_LEFT][2]};
	for (int i = 0; i < 3; ++i)
		if (DOT(normals[i], vec) >= r)					// normals are already normalized
			return false;
	vec[0] = c[0]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][0];
	vec[1] = c[1]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][1];
	vec[2] = c[2]-vertices[NGIN_FRUSTUM_BACK_TOP_RIGHT][2];
	for (int i = 3; i < 6; ++i)
		if (DOT(normals[i], vec) >= r)
			return false;
	return true;
}

shNginFrustum &shNginFrustum::operator =(const shNginFrustum &RHS)
{
	memcpy(vertices, RHS.vertices, sizeof(vertices));
	memcpy(normals, RHS.normals, sizeof(normals));
	return *this;
}
