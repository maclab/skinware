/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of Ngin3d.
 *
 * Ngin3d is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ngin3d is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ngin3d.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Ngin3d_physics.h"

#define				SHMACROS_LOG_PREFIX			"Ngin3d"

#include "shmacros.h"

shNgin3dPhysicalObject::shNgin3dPhysicalObject()
{
	position[0] = position[1] = position[2] = 0;
	speed[0] = speed[1] = speed[2] = 0;
	acceleration[0] = acceleration[1] = acceleration[2] = 0;
}

shNgin3dPhysicalObject::shNgin3dPhysicalObject(float pos[3])
{
	position[0] = pos[0];
	position[1] = pos[1];
	position[2] = pos[2];
	speed[0] = speed[1] = speed[2] = 0;
	acceleration[0] = acceleration[1] = acceleration[2] = 0;
}

shNgin3dCollisionResult shNgin3dPhysicalObject::shNgin3dPOUpdate(unsigned long dt, float controllableSpeed[3])
{
	shNgin3dCollisionResult res = NGIN3D_NO_COLLISION;
	float t = dt/1000.0f;
	// TODO: gravity can be incorporated in acceleration. However, collision must be checked. In case of collision, movement in a certain direction would not be completely possible as well as the speed in that direction being set
	// to zero. Of course, this direction could not be simply x, y or z. Imagine jumping up against a ceiling that has a slope of 45 degrees. The player would not be stopped when hitting the ceiling, but it would be redirected,
	// however with a fraction of its speed.
	position[0] += (speed[0]+controllableSpeed[0])*t;
	position[1] += (speed[1]+controllableSpeed[1])*t;
	position[2] += (speed[2]+controllableSpeed[2])*t;
	speed[0] += acceleration[0]*t;
	speed[1] += acceleration[1]*t;
	speed[2] += acceleration[2]*t;
/*	if (position[1] < 0)				// TODO: this is temporary. Only means that the player is walking on a surface at y = 0
	{
		position[1] = 0;
		speed[1] = 0;
		res = NGIN3D_COLLISION_WITH_GROUND;
	}*/
	return res;
}
