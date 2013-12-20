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

#include <math.h>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include "triangulate.h"
#include "vecmath.h"

using namespace std;

static float			extension		= 0.3;
static float			min_max_diff		= 1.9;
static float			nearest_max_diff	= 1.9;
extern ofstream			fout;

#define POSITION(x)	(sensors[(x)].relative_position)
#define NORMAL(x)	(sensors[(x)].relative_orientation)

#define SQRT3		(1.732050808f)

vector<triangle> triangulate(skin_module *module)
{
	// Step 1: For each sensor a, find its closest sensor that has distance greater than 0. If no such sensor exists, do the following;
	//             Assume normal(a) has form [x, y, z]
	//             Find a-non zero coordinate and create a perpendecular vector p. For example: [-y, x, 0] or [0, -z, y] or [z, 0, -x]
	//             If sensors normal is zero, assume (0, 0, 1)
	//             Find the other perpendecular vector p2=normal(a)xp
	//             Find four points:
	//             ar = a+p*extension, al = a-p*extension
	//             au = a+p2*extension, ad = a-p2*extension
	//             Create two triangles with coordinates <ar, au, al> and <ar, al, ad> (the order is already correct)
	// Step 2: For each three sensors a, b and c, see if these condition hold:
	//             ||a-b|| > 0 && ||a-c|| > 0 && ||b-c|| > 0
	//             ((a-b)/||a-b||)x((a-c)/||a-c||) != 0
	//             max(||a-b||, ||a-c||, ||b-c||)/min(||a-b||, ||a-c||, ||b-c||) < min_max_diff
	//             max(||a-b||, ||a-c||, ||b-c||)/min(||a-closest(a)||, ||b-closest(b)||, ||c-closest(c)||) < nearest_max_diff
	//         If the conditions hold, create a triangle from points p1, p2, and p3 according to the following algorithm:
	//             mab = (a+b)/2, mac = (a+c)/2, mbc = (b+c)/2
	//             nab = (mab-c)*(1+extension)+c, nac = (mac-b)*(1+extension)+b, nbc = (mbc-a)*(1+extension)+a
	//             t1(a-b)+nab=t2(a-c)+nac => solve for t1 and t2 => p1 = t1(a-b)+nab
	//             t1(b-c)+nbc=t2(b-a)+nab => solve for t1 and t2 => p2 = t1(b-c)+nbc
	//             t1(c-a)+nac=t2(c-b)+nbc => solve for t1 and t2 => p3 = t1(c-a)+nac
	//             Note that since we know the lines intersect, only 2 dimensions out of 3 are enough to compute t1 and t2.
	//                 Choose two dimensions that at least one of them is not zero
	//             Note also that computing only t1 is enough.
	//             Put p1, p2 and p3 in such an order that (p1-p3)x(p2-p1) would have the same sign as p1 (or p2 or p3 if any normal was 0)
	// Step 3: See which sensors have not been a part of any triangle. For each of those sensors a, do the following:
	//             Let b be the closest sensor to a
	//             na = (a-b)*(1+extension)+b, nb = (b-a)*(1+extension)+a
	//             dirap = ((a-b)xnormal(a))/||(a-b)xnormal(a)||
	//             dirbp = ((b-a)xnormal(b))/||(b-a)xnormal(b)||
	//             Note that if any normal is 0, use the other one. If both are zero, assume (0, 0, 1)
	//             nal = na-extension*dirap, nar = na+extension*dirap
	//             nbl = nb+extension*dirbp, nbr = na-extension*dirbp
	//             Create two triangles <nbr, nar, nal> and <nbr, nal, nbl> (the order of points is already correct)
	vector<triangle>	mesh;
	skin_sensor_size	sensors_count;
	skin_sensor		*sensors	= skin_module_sensors(module, &sensors_count);
	vector<skin_sensor_id>	closests(sensors_count, (skin_sensor_id)SKIN_INVALID_ID);
	vector<bool>		covered(sensors_count, false);		// a flag to make sure the sensor is covered. If not covered, connect it to its closest neighbor
	// Step 1
	for (skin_sensor_id i = 0; i < sensors_count; ++i)
	{
		skin_sensor_id	closest		= (skin_sensor_id)SKIN_INVALID_ID;
		float		min_dist	= 0.0f;
		for (skin_sensor_id j = 0; j < sensors_count; ++j)
		{
			if (i == j)
				continue;
			float diff[3];
			SUB(diff, POSITION(i), POSITION(j));
			float dist = NORM(diff);
			if (ZERO(dist))
				continue;
			if (closest == (skin_sensor_id)SKIN_INVALID_ID || dist < min_dist)
			{
				min_dist = dist;
				closest = j;
			}
		}
		if (closest == (skin_sensor_id)SKIN_INVALID_ID)
		{
			// Lone sensor (Means either all sensors of module are too close (!!) or module has only one sensor)
			float	normal[3]	= {0, 0, 1};
			if (!VZERO(NORMAL(i)))
				ASSIGN(normal, NORMAL(i));
			float p[3] = {0, 0, 0};
			for (int j = 0; j < 3; ++j)
				if (!ZERO(normal[j]))	// should happen for at least one of them
				{
					p[j] = -normal[(j + 1) % 3];
					p[(j + 1) % 3] = normal[j];
					break;
				}
			float p2[3];
			CROSS(p2, normal, p);
			// Note: there are no information on module scales. In reality however, the distance between modules is around roughly 5mm
			// The extension is applied to a distance of 10mm in the hope that it would make sense! This number is totally experimental
			float	estimatedextension	= extension / 100;
			float	a[3], ar[3], al[3], au[3], ad[3], temp[3];
			ASSIGN(a, POSITION(i));
			triangle t;
			ASSIGN(t.normal, normal);
			MUL(temp, p, estimatedextension);
			ADD(ar, a, temp);
			SUB(al, a, temp);
			MUL(temp, p2, estimatedextension);
			ADD(au, a, temp);
			SUB(ad, a, temp);
			ASSIGN(t.p1, ar);
			ASSIGN(t.p2, au);
			ASSIGN(t.p3, al);
			mesh.push_back(t);
			ASSIGN(t.p2, al);
			ASSIGN(t.p3, ad);
			mesh.push_back(t);
			covered[i] = true;
		}
		closests[i] = closest;
	}
	// Step 2
	for (skin_sensor_id i = 0; i < sensors_count-2; ++i)
	{
		if (closests[i] == (skin_sensor_id)SKIN_INVALID_ID) continue;
		float			a[3];
		ASSIGN(a, POSITION(i));
		for (skin_sensor_id j = i+1; j < sensors_count-1; ++j)
		{
			if (closests[j] == (skin_sensor_id)SKIN_INVALID_ID) continue;
			float		b[3], ba[3],
					mab[3];
			ASSIGN(b, POSITION(j));
			SUB(ba, a, b);
			float		dba = NORM(ba);
			if (ZERO(dba)) continue;
			ADD(mab, a, b);
			DIV(mab, mab, 2);
			for (skin_sensor_id k = j+1; k < sensors_count; ++k)
			{
				if (closests[k] == (skin_sensor_id)SKIN_INVALID_ID) continue;
				float	c[3], cb[3], ac[3],
					mbc[3], mca[3],
					nab[3], nbc[3], nca[3],
					temp[3], temp2[3];
				ASSIGN(c, POSITION(k));
				// Check conditions to see if they make a good triangle
				SUB(cb, b, c);
				SUB(ac, c, a);
				float	d		= dba;
				float	max_dist	= d;
				float	min_dist	= d;
				if (ZERO(d)) continue;
				DIV(temp, ba, dba);
				d = NORM(cb);
				if (ZERO(d)) continue;
				DIV(temp2, cb, d);
				if (d > max_dist) max_dist = d;
				if (d < min_dist) min_dist = d;
				d = NORM(ac);
				if (ZERO(d)) continue;
				if (d > max_dist) max_dist = d;
				if (d < min_dist) min_dist = d;
				CROSS(temp, temp, temp2);
				if (VZERO(temp)) continue;
				if (max_dist > min_dist*min_max_diff) continue;
				SUB(temp, a, POSITION(closests[i]));
				min_dist = NORM(temp);
				SUB(temp, b, POSITION(closests[j]));
				d = NORM(temp);
				if (min_dist < d) min_dist = d;
				SUB(temp, c, POSITION(closests[k]));
				d = NORM(temp);
				if (min_dist < d) min_dist = d;
				if (max_dist > min_dist*nearest_max_diff) continue;
				// Create the enlarged triangle
				ADD(mbc, b, c);
				DIV(mbc, mbc, 2);
				ADD(mca, c, a);
				DIV(mca, mca, 2);
				SUB(nab, mab, c);
				MUL(nab, nab, 1+extension);
				ADD(nab, nab, c);
				SUB(nbc, mbc, a);
				MUL(nbc, nbc, 1+extension);
				ADD(nbc, nbc, a);
				SUB(nca, mca, b);
				MUL(nca, nca, 1+extension);
				ADD(nca, nca, b);
				triangle t;
				int	d1, d2;
				bool found = false;
				for (d1 = 0; d1 < 2 && !found; ++d1)
					for (d2 = d1+1; d2 < 3 && ! found; ++d2)
						if ((!ZERO(ba[d1]) && !ZERO(ac[d2])) || (!ZERO(ba[d2]) && !ZERO(ac[d1])))
							found = true;
				--d1; --d2;
				if (!found) continue;				// shouldn't happen!
				float	p1top[2][2]	= {{nab[d1] - nca[d1], ac[d1]}, {nab[d2] - nca[d2], ac[d2]}};
				float	p1bottom[2][2]	= {{ba[d1], ac[d1]}, {ba[d2], ac[d2]}};
				float	detbottom	= DET2(p1bottom);
				if (ZERO(detbottom)) continue;			// shouldn't happen
				float	t1		= DET2(p1top) / detbottom;
				MUL(t.p1, ba, t1);
				ADD(t.p1, t.p1, nab);
				found = false;
				for (d1 = 0; d1 < 2 && !found; ++d1)
					for (d2 = d1+1; d2 < 3 && ! found; ++d2)
						if ((!ZERO(cb[d1]) && !ZERO(ba[d2])) || (!ZERO(cb[d2]) && !ZERO(ba[d1])))
							found = true;
				--d1; --d2;
				if (!found) continue;				// shouldn't happen!
				float	p2top[2][2]	= {{nbc[d1] - nab[d1], ba[d1]}, {nbc[d2] - nab[d2], ba[d2]}};
				float	p2bottom[2][2]	= {{cb[d1], ba[d1]}, {cb[d2], ba[d2]}};
				detbottom		= DET2(p2bottom);
				if (ZERO(detbottom)) continue;			// shouldn't happen
				t1			= DET2(p2top) / detbottom;
				MUL(t.p2, cb, t1);
				ADD(t.p2, t.p2, nbc);
				found = false;
				for (d1 = 0; d1 < 2 && !found; ++d1)
					for (d2 = d1+1; d2 < 3 && ! found; ++d2)
						if ((!ZERO(ac[d1]) && !ZERO(cb[d2])) || (!ZERO(ac[d2]) && !ZERO(cb[d1])))
							found = true;
				--d1; --d2;
				if (!found) continue;				// shouldn't happen!
				float	p3top[2][2]	= {{nca[d1] - nbc[d1], cb[d1]}, {nca[d2] - nbc[d2], cb[d2]}};
				float	p3bottom[2][2]	= {{ac[d1], cb[d1]}, {ac[d2], cb[d2]}};
				detbottom		= DET2(p3bottom);
				if (ZERO(detbottom)) continue;			// shouldn't happen
				t1			= DET2(p3top) / detbottom;
				MUL(t.p3, ac, t1);
				ADD(t.p3, t.p3, nca);
				SUB(temp, t.p1, t.p3);
				SUB(temp2, t.p2, t.p1);
				CROSS(temp, temp, temp2);
				ASSIGN(t.normal, temp);
				bool	swap		= false;
				if (!VZERO(NORMAL(i)))
					swap = DOT(temp, NORMAL(i)) < 0;
				else if (!VZERO(NORMAL(j)))
					swap = DOT(temp, NORMAL(j)) < 0;
				else if (!VZERO(NORMAL(k)))
					swap = DOT(temp, NORMAL(k)) < 0;
				else continue;					// shouldn't happen
				if (swap)
				{
					ASSIGN(temp, t.p2);
					ASSIGN(t.p2, t.p3);
					ASSIGN(t.p3, temp);
					MUL(t.normal, t.normal, -1);
				}
				float	norm		= NORM(t.normal);
				if (ZERO(norm))					// shouldn't happen
				{
					float up[3]	= {0, 0, 1};
					ASSIGN(t.normal, up);
				}
				DIV(t.normal, t.normal, norm);
				mesh.push_back(t);
				covered[i] = true;
				covered[j] = true;
				covered[k] = true;
			}
		}
	}
	// Step 3
	typedef pair<skin_sensor_id, skin_sensor_id> PAIR;
	set<PAIR>	already_built;
	float		ext			= extension;
	if (ext < 0.15f)
		ext = 0.15f;
	for (skin_sensor_id i = 0; i < sensors_count; ++i)
	{
		if (covered[i] || closests[i] == (skin_sensor_id)SKIN_INVALID_ID) continue;
		if (already_built.find(PAIR(closests[i], i)) != already_built.end()) continue;	// already done
		float	a[3], b[3], ba[3],
			normala[3], normalb[3],
			na[3], nb[3],
			dirap[3], dirbp[3],
			nal[3], nar[3], nbl[3], nbr[3];
		ASSIGN(a, POSITION(i));
		ASSIGN(b, POSITION(closests[i]));
		SUB(ba, a, b);
		ASSIGN(normala, NORMAL(i));
		ASSIGN(normalb, NORMAL(closests[i]));
		if (VZERO(normala) && VZERO(normalb))
		{
			float up[3] = {0, 0, 1};
			ASSIGN(normala, up);
			ASSIGN(normalb, up);
		}
		else if (VZERO(normala))
			ASSIGN(normala, normalb);
		else if (VZERO(normalb))
			ASSIGN(normalb, normala);
		MUL(na, ba, 1 + ext);
		ADD(na, na, b);
		MUL(nb, ba, -(1 + ext));
		ADD(nb, nb, a);
		CROSS(dirap, ba, normala);
		if (VZERO(dirap)) continue;		// shouldn't happen
		CROSS(dirbp, ba, normalb);
		if (VZERO(dirbp)) continue;		// shouldn't happen
		DIV(dirbp, dirbp, -1);
		// Compute the 4 surrounding points
		MUL(dirap, dirap, ext);
		MUL(dirbp, dirbp, ext);
		SUB(nal, na, dirap);
		ADD(nar, na, dirap);
		ADD(nbl, nb, dirbp);
		SUB(nbr, nb, dirbp);
		triangle t;
		ASSIGN(t.p1, nbr);
		ASSIGN(t.p2, nar);
		ASSIGN(t.p3, nal);
		ASSIGN(t.normal, normala);
		mesh.push_back(t);
		ASSIGN(t.p2, nal);
		ASSIGN(t.p3, nbl);
		mesh.push_back(t);
		already_built.insert(PAIR(i, closests[i]));
	}
	return mesh;
}

#define CHECK_AND_GET(x) if (s == #x) x = v

int read_triangulation_settings(const char *file)
{
	ifstream fin(file);
	if (!fin.is_open())
	{
		fout << "Could not open file \"" << file << "\"" << endl;
		return -1;
	}
	string s;
	string c;
	float v;
	while (fin >> s >> c >> v)
	{
		CHECK_AND_GET(extension);
		CHECK_AND_GET(min_max_diff);
		CHECK_AND_GET(nearest_max_diff);
	}
	return 0;
}
