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

#ifndef NGIN3D_BASE_H_BY_CYCLOPS
#define NGIN3D_BASE_H_BY_CYCLOPS

typedef void (*Ngin3d_setCameraPosition)(float p[3]);
typedef void (*Ngin3d_setCameraTarget)(float p[3]);
typedef void (*Ngin3d_setCameraUp)(float p[3]);

// Initialization and finalization
void shNgin3dInitialize();
void shNgin3dQuit();

// Interface to manipulate camera. Plugable to Ngin without need for extra function definitoins. Simply put shNgin3dCameraPositionFunction(shNginSetCameraPositionv) for example.
Ngin3d_setCameraPosition shNgin3dCameraPositionFunction(Ngin3d_setCameraPosition f);
Ngin3d_setCameraTarget shNgin3dCameraTargetFunction(Ngin3d_setCameraTarget f);
Ngin3d_setCameraUp shNgin3dCameraUpFunction(Ngin3d_setCameraUp f);

// Movement
void shNgin3dFreefly();				// Free fly mode. Up can be anything and the movements are relative to top
void shNgin3dUpright();				// Upright mode. Up can only be in the same plane as target and 0, 1, 0
void shNgin3dUpsideDownPossible();		// It's possible to look upside down
void shNgin3dUpsideDownNotPossible();		// It's impossible to look upside down
void shNgin3dMove(float frontSpeed, float rightSpeed, float upSpeed, unsigned long dt);
						// Move the position of the camera relative to TARGETxRIGHTxUP coordinate system. If in upright mode, y would affect in 0, 1(-1), 0 direction not up
						// dt given in miliseconds
void shNgin3dTwist(float x, float y);		// If in free fly mode: relative to frame RIGHTxUP coordinate system (remember that target direction, right and up are all normalized vectors)
						// TODO: in free fly mode, x*right+y*up is added to target and then normalized. This is approximate but works fine if movement is not too fast. What is too fast? Dunno yet!
						// If in upright mode: x (degrees) for longitude and y (degrees) for latitude (in direction of up which is in the same plane as target and 0, 1, 0)
void shNgin3dRoll(float deg);			// Only in free fly mode. Rotate around target direction deg degrees in clockwise direction
void shNgin3dPlacePlayer(float position[3]);	// place the player in this location

// Initialization and finalization
void shNgin3dInitialize();
void shNgin3dQuit();

// Getter functions
void shNgin3dGetCamera(float position[3], float target[3], float up[3]);

#endif
