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

#ifndef NGIN3D_ACTIONS_H_BY_CYCLOPS
#define NGIN3D_ACTIONS_H_BY_CYCLOPS

enum shNgin3dAction
{
	NGIN3D_NO_ACTION = 0,
	NGIN3D_LOOK_AROUND = 1,			// Registered with shNgin3dInput::NGIN3D_MOUSE_MOVE and cannot be changed (Ngin3d_input.h)
	NGIN3D_MOVE_LEFT = 2,
	NGIN3D_MOVE_RIGHT = 3,
	NGIN3D_MOVE_FORWARD = 4,
	NGIN3D_MOVE_BACKWARD = 5,
	NGIN3D_JUMP = 6,			// gives an instant boost upwards to player's speed, gravity will take it down
	NGIN3D_CROUCH = 7,			// doesn't change position, only camera gets lower
	NGIN3D_ROLL_RIGHT = 8,
	NGIN3D_ROLL_LEFT = 9,
	NGIN3D_FLY_UP = 10,			// constant speed upwards/downwards
	NGIN3D_FLY_DOWN = 11
	// TODO: Add support for fire and alternate fire. For each, there have to be registered an object (A set of functions need to be registered to handle the object: create (return id), destroy (get id) and move (get id and
	// displacement) (which returns whether it collided with anything) at least!) and whether they are affected by gravity. Ngin (currently version 0.2.0) doesn't have these functions yet, but they will.
	// TODO: Add support for change weapon
};

enum shNgin3dSetting
{
	NGIN3D_FORWARD_SPEED = 0,
	NGIN3D_BACKWARD_SPEED,
	NGIN3D_STRAFE_SPEED,
	NGIN3D_ASCEND_SPEED,
	NGIN3D_DESCEND_SPEED,
	NGIN3D_CROUCH_SPEED_FACTOR,				// affects FORWARD_SPEED, BACKWARD_SPEED and STRAFE_SPEED
	NGIN3D_IN_AIR_SPEED_FACTOR,				// affects FORWARD_SPEED, BACKWARD_SPEED and STRAFE_SPEED while jumping
	NGIN3D_JUMP_INITIAL_SPEED,
	NGIN3D_ROLL_SPEED,
	NGIN3D_YAW_SPEED,
	NGIN3D_PITCH_SPEED,
	NGIN3D_HEIGHT,						// height of the player
	NGIN3D_CROUCHED_HEIGHT,					// height of the player when crouched
	NGIN3D_GRAVITY_X,
	NGIN3D_GRAVITY_Y,
	NGIN3D_GRAVITY_Z					// normally, NGIN3D_GRAVITY_Y is used, but in case of fictional change in gravity, it could have values in x and z direction also.
};

void shNgin3dReadSettings(const char *file);			// Read data from file. See documentation for file format and possible data
void shNgin3dSetSetting(shNgin3dSetting s, float v);		// Set setting s to value v
void shNgin3dEffectuate(unsigned int dt);			// After all the input is given, this function MUST be called for them to take effect.
								// What it does is basically combine the effect of actions of all inputs and call the proper functions
								// dt is time passed since last shNgin3dEffectuate call and is in milliseconds.

// The following functions exist for each action in Ngin3d_input.h. The bool inputs indicate whether those actions are starting or finishing.
// These functions are called by shNgin3dInput and are internal to Ngin3d. No need to call them.
void shNgin3dMoveLeft(bool);
void shNgin3dMoveRight(bool);
void shNgin3dMoveForward(bool);
void shNgin3dMoveBackward(bool);
void shNgin3dJump(bool);
void shNgin3dCrouch(bool);
void shNgin3dLookAround(int, int);
void shNgin3dRollLeft(bool);
void shNgin3dRollRight(bool);
void shNgin3dFlyUp(bool);
void shNgin3dFlyDown(bool);

void shNing3dResetActionStates();					// Used when you want all actions to be reset. For example, level finished and the player was moving, so now needs to be stopped for next level

#endif
