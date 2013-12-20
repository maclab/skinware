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

#ifndef NGIN3D_INPUT_H_BY_CYCLOPS
#define NGIN3D_INPUT_H_BY_CYCLOPS

#include "Ngin3d_keydef.h"
#include "Ngin3d_actions.h"

enum shNgin3dInput
{
	NGIN3D_NO_INPUT = 0,
	NGIN3D_MOUSE_MOVE = 1,			// Registered with shNgin3dAction::NGIN3D_LOOK_AROUND and cannot be changed
	NGIN3D_MOUSE_LEFT = 2,
	NGIN3D_MOUSE_RIGHT = 3,
	NGIN3D_MOUSE_MIDDLE = 4,
	NGIN3D_KEY = 5,				// Accompanied by key code
	NGIN3D_MOUSE_MIDDLE_SCROLL_UP = 6,
	NGIN3D_MOUSE_MIDDLE_SCROLL_DOWN = 7
};

#define NGIN3D_PRESSED true
#define NGIN3D_RELEASED false

void shNgin3dRegisterAction(shNgin3dInput input, shNgin3dAction action, shNgin3dKey key = NGIN3D_NO_KEY);	// Whenever input is received, action will happen. In case of NGIN3D_KEY, the input is coupled with key
void shNgin3dReceivedInput(shNgin3dInput input, bool pressed = NGIN3D_PRESSED);					// The action assigned to input is executed. pressed indicates whether the input is "pressed" or "released". Which means
														// if it is starting to take effect or stoping to.
void shNgin3dReceivedInput(shNgin3dInput input, shNgin3dKey key, bool pressed = NGIN3D_PRESSED);		// Input MUST be NGIN3D_KEY. The input is coupled with key. pressed shows whether the key is pressed or released
void shNgin3dReceivedInput(shNgin3dInput input, int dx, int dy);						// input MUST be NGIN3D_MOUSE_MOVE. dx and dy represent the movement of the mouse
//void shNgin3dEffectuate(unsigned int dt);									// After all the input is given, this function MUST be called for them to take effect.
														// What it does is basically combine the effect of actions of all inputs and call the proper functions
														// This function is defined in Ngin3d_actions.h
void shNgin3dUnregisterInputActions();

#endif
