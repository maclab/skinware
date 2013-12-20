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

#ifndef NGIN_OBJECT_H_BY_CYCLOPS
#define NGIN_OBJECT_H_BY_CYCLOPS

#include <vector>

using namespace std;

#define				NGIN_INVALID_OBJECT			((unsigned int)-1)
#define				NGIN_NO_COLLISION			-1

class shNginObject
{
private:
	unsigned int id;			// NGIN_INVALID_OBJECT if not built
public:
	struct shNginOSphere { float center[3]; float radius; };
	struct shNginOSection { int id; shNginOSphere section; };
	shNginOSphere boundary;
	vector<shNginOSection> sections;	// sections are smaller spheres together approximating the shape of the object (could be used instead of boundary to detect
						// collision with frustum) with the main purpose of collision detection. If divided in a meaningful way they can also provide
						// additional effects, such as bloodying the shot limb or smoke rising from the destroyed body part.
	shNginObject();
	shNginObject(const shNginObject &RHS);	// A display list cannot be copied
	~shNginObject();
	void shNginOBeginCreating();
	void shNginOEndCreating();
	void shNginODraw() const;
	void shNginOAddSection(const shNginOSection &sec);
	void shNginOAddSection(int id, const shNginOSphere &sphere);
	void shNginOAddSection(int id, float *c, float r);
	bool shNginOIsCreated() const;
	void shNginOFree();			// Assumes the object is already created, use shSGLOIsCreated() before
};

class shNginObjectInstance
{
public:
	const shNginObject *object;
	float transformationMatrix[16];		// This is not to store the transformation needed to draw the object, but is to store what transformation actually took place
						// and is used in shNginOICheckCollision() to determine the global positions of the boundary and shNginObject::shNginOSections
	shNginObjectInstance();
	shNginObjectInstance(const shNginObject &obj);
	~shNginObjectInstance();
	int shNginOICheckCollision(const shNginObjectInstance &other, int *otherpart = '\0') const;	// return NGIN_NO_COLLISION if they don't collide, or an index to sections
													// vector indicating which section "other" collided with. If otherpart is
													// provided, in that it is also returned which part of "other" collided with
													// the object (or NGIN_NO_COLLISION if no collision)
};

#endif
