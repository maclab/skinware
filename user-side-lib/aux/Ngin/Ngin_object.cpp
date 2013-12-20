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

#include "Ngin_object.h"
#include "Ngin_opengl.h"

#define				SHMACROS_LOG_PREFIX			"Ngin"

#include "shmacros.h"

shNginObject::shNginObject()
{
	id = NGIN_INVALID_OBJECT;
	sections.clear();
}

shNginObject::shNginObject(const shNginObject &RHS)
{
	id = NGIN_INVALID_OBJECT;
	sections.clear();
	// Cannot take a copy of display list from openGL so a copy is not allowed
	LOG("Illagal copy of shNginObject: Cannot copy objects, must pass pointers or references");
}

shNginObject::~shNginObject()
{
	if (shNginOIsCreated())
		shNginOFree();
}

void shNginObject::shNginOBeginCreating()
{
	if (shNginOIsCreated())
		shNginOFree();
	id = glGenLists(1);
	glNewList(id, GL_COMPILE);
}

void shNginObject::shNginOEndCreating()
{
	glEndList();
}

void shNginObject::shNginODraw() const
{
	if (shNginOIsCreated())
		glCallList(id);
}

void shNginObject::shNginOAddSection(const shNginOSection &sec)
{
	sections.push_back(sec);
}

void shNginObject::shNginOAddSection(int id, const shNginOSphere &sphere)
{
	shNginOSection sec;
	sec.id = id;
	sec.section = sphere;
	sections.push_back(sec);
}

void shNginObject::shNginOAddSection(int id, float *c, float r)
{
	shNginOSection sec;
	sec.id = id;
	sec.section.center[0] = c[0];
	sec.section.center[1] = c[1];
	sec.section.center[2] = c[2];
	sec.section.radius = r;
	sections.push_back(sec);
}

bool shNginObject::shNginOIsCreated() const
{
	return (id != NGIN_INVALID_OBJECT);
}

void shNginObject::shNginOFree()
{
	glDeleteLists(id, 1);
	id = NGIN_INVALID_OBJECT;
	sections.clear();
}

shNginObjectInstance::shNginObjectInstance()
{
	object = '\0';
	transformationMatrix[0] = 1; transformationMatrix[1] = 0; transformationMatrix[2] = 0; transformationMatrix[3] = 0;
	transformationMatrix[4] = 0; transformationMatrix[5] = 1; transformationMatrix[6] = 0; transformationMatrix[7] = 0;
	transformationMatrix[8] = 0; transformationMatrix[9] = 0; transformationMatrix[10] = 1; transformationMatrix[11] = 0;
	transformationMatrix[12] = 0; transformationMatrix[13] = 0; transformationMatrix[14] = 0; transformationMatrix[15] = 1;
}

shNginObjectInstance::shNginObjectInstance(const shNginObject &obj)
{
	object = &obj;
	transformationMatrix[0] = 1; transformationMatrix[1] = 0; transformationMatrix[2] = 0; transformationMatrix[3] = 0;
	transformationMatrix[4] = 0; transformationMatrix[5] = 1; transformationMatrix[6] = 0; transformationMatrix[7] = 0;
	transformationMatrix[8] = 0; transformationMatrix[9] = 0; transformationMatrix[10] = 1; transformationMatrix[11] = 0;
	transformationMatrix[12] = 0; transformationMatrix[13] = 0; transformationMatrix[14] = 0; transformationMatrix[15] = 1;
}

shNginObjectInstance::~shNginObjectInstance()
{
}

#define				TRANSFORM(x, y, res)			(res)[0] = (y)[0]*(x)[0]+(y)[4]*(x)[1]+(y)[8]*(x)[2]+(y)[12];\
									(res)[1] = (y)[1]*(x)[0]+(y)[5]*(x)[1]+(y)[9]*(x)[2]+(y)[13];\
									(res)[2] = (y)[2]*(x)[0]+(y)[6]*(x)[1]+(y)[10]*(x)[2]+(y)[14]

int shNginObjectInstance::shNginOICheckCollision(const shNginObjectInstance &other, int *otherpart) const
{
	// Working with distance to the power of 2, to avoid sqrt
	if (object == '\0' || other.object == '\0')
	{
		if (otherpart)
			*otherpart = NGIN_NO_COLLISION;
		return NGIN_NO_COLLISION;
	}
	const shNginObject &obj = *object;
	const shNginObject &otherobj = *other.object;
	float temp1[3], temp2[3];
	TRANSFORM(obj.boundary.center, transformationMatrix, temp1);
	TRANSFORM(otherobj.boundary.center, other.transformationMatrix, temp2);
	float dist2 = DISTANCE2(temp1, temp2);
	if (dist2 > (obj.boundary.radius+otherobj.boundary.radius)*(obj.boundary.radius+otherobj.boundary.radius))
	{
		if (otherpart)
			*otherpart = NGIN_NO_COLLISION;
		return NGIN_NO_COLLISION;
	}
	int secsize = (int)obj.sections.size();
	int othersize = (int)otherobj.sections.size();
	for (int i = 0; i < secsize; ++i)
		for (int j = 0; j < othersize; ++j)
		{
			const shNginObject::shNginOSphere &sp = obj.sections[i].section;
			const shNginObject::shNginOSphere &othersp = otherobj.sections[j].section;
			TRANSFORM(sp.center, transformationMatrix, temp1);
			TRANSFORM(othersp.center, other.transformationMatrix, temp2);
			dist2 = DISTANCE2(temp1, temp2);
			if (dist2 < (sp.radius+othersp.radius)*(sp.radius+othersp.radius))
			{
				if (otherpart)
					*otherpart = j;
				return i;
			}
		}
	if (otherpart)
		*otherpart = NGIN_NO_COLLISION;
	return NGIN_NO_COLLISION;
}
