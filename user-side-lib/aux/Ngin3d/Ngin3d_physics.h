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

#ifndef NGIN3D_PHYSICS_H_BY_CYCLOPS
#define NGIN3D_PHYSICS_H_BY_CYCLOPS

enum shNgin3dCollisionResult
{
	NGIN3D_NO_COLLISION = 0,
	NGIN3D_COLLISION_WITH_GROUND
};

class shNgin3dPhysicalObject
{
public:
	float position[3];
	float speed[3];						// This speed is not controllable (falling, being thrown, (initial jump speed is added also here) etc)
	float acceleration[3];
	shNgin3dPhysicalObject();
	shNgin3dPhysicalObject(float pos[3]);
	shNgin3dCollisionResult shNgin3dPOUpdate(unsigned long dt, float controllableSpeed[3]);		// dt in miliseconds. Controllable speed is speed imposed through input
};

#endif
