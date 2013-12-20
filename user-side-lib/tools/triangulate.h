/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRIANGULATE_H_BY_CYCLOPS
#define TRIANGULATE_H_BY_CYCLOPS

#include <vector>
#include <skin.h>

struct triangle			// guaranteed to be counter clockwise and not on a straight line
{
	float p1[3];		// positions would be
	float p2[3];		// relative to
	float p3[3];		// robot link
	float normal[3];
};

int read_triangulation_settings(const char *file);
std::vector<triangle> triangulate(skin_module *module);

#define EXPANDED(x)	{(x)[0], (x)[1], (x)[2]}
#define EXPANDED2(x)	{{(x)[0][0], (x)[0][1]}, {(x)[1][0], (x)[1][1]}}
#define EXPANDED3(x)	{{(x)[0][0], (x)[0][1], (x)[0][2]}, {(x)[1][0], (x)[1][1], (x)[1][2]}, {(x)[2][0], (x)[2][1], (x)[2][2]}}
#define EPSILON		(1e-15f)
#define LARGENUM	(1e15f)
#define ZERO(x)		({ typeof(x) _x = (x); _x < EPSILON && _x > -EPSILON; })
#define VZERO(x)	({ typeof(x) _xx = EXPANDED(x);  ZERO(_xx[0]) && ZERO(_xx[1]) && ZERO(_xx[2]); })
#define NORM(x)		({ typeof(x) _x = EXPANDED(x); sqrt(_x[0]*_x[0]+_x[1]*_x[1]+_x[2]*_x[2]); })
#define ASSIGN(res,a)	do { typeof(a) _a = EXPANDED(a); (res)[0]=_a[0]; (res)[1]=_a[1]; (res)[2]=_a[2]; } while(0)
#define ADD(res,a,b)	do { typeof(a) _a = EXPANDED(a); typeof(b) _b = EXPANDED(b); (res)[0]=_a[0]+_b[0]; (res)[1]=_a[1]+_b[1]; (res)[2]=_a[2]+_b[2]; } while(0)
#define SUB(res,a,b)	do { typeof(a) _a = EXPANDED(a); typeof(b) _b = EXPANDED(b); (res)[0]=_a[0]-_b[0]; (res)[1]=_a[1]-_b[1]; (res)[2]=_a[2]-_b[2]; } while(0)
#define MUL(res,a,c)	do { typeof(a) _a = EXPANDED(a); typeof(c) _c = (c); (res)[0]=_a[0]*_c; (res)[1]=_a[1]*_c; (res)[2]=_a[2]*_c; } while(0)
#define DIV(res,a,c)	do { typeof(a) _a = EXPANDED(a); typeof(c) _c = (c); (res)[0]=_a[0]/_c; (res)[1]=_a[1]/_c; (res)[2]=_a[2]/_c; } while(0)
#define DISTANCE(a,b)	({ typeof(a) _x; SUB(_x,a,b); sqrt(_x[0]*_x[0]+_x[1]*_x[1]+_x[2]*_x[2]); })
#define DET2(a)		({ typeof(a) _a = EXPANDED2(a); _a[0][0]*_a[1][1]-_a[0][1]*_a[1][0]; })
#define DET3(a)		({ typeof(a) _a = EXPANDED3(a); _a[0][0]*_a[1][1]*_a[2][2]+_a[1][0]*_a[2][1]*_a[0][2]+_a[2][0]*_a[0][1]*_a[1][2]+\
							-(_a[0][0]*_a[2][1]*_a[1][2]+_a[1][0]*_a[0][1]*_a[2][2]+_a[2][0]*_a[1][1]*_a[0][2]); })
#define DOT(a,b)	({ typeof(a) _a = EXPANDED(a); typeof(b) _b = EXPANDED(b); _a[0]*_b[0]+_a[1]*_b[1]+_a[2]*_b[2]; })
#define CROSS(res,a,b)	do { typeof(a) _a = EXPANDED(a); typeof(b) _b = EXPANDED(b);\
				(res)[0]=_a[1]*_b[2]-_a[2]*_b[1]; (res)[1]=_a[2]*_b[0]-_a[0]*_b[2]; (res)[2]=_a[0]*_b[1]-_a[1]*_b[0]; } while(0)

// For debugging
#if 1
#define DUMP(x)		do { cout << __FILE__"(" << __LINE__ << "): " << #x << ": " << (x) << endl; } while (0)
#define VDUMP(x)	do { cout << __FILE__"(" << __LINE__ << "): " << #x << ": " << (x)[0] << " " << (x)[1] << " " << (x)[2] << endl; } while (0)
#else
#define DUMP(x)		((void)0)
#define VDUMP(x)	((void)0)
#endif

#endif
