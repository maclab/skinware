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

#include "Ngin3d_input.h"
#include "Ngin3d_actions.h"
#include <string.h>

#define				SHMACROS_LOG_PREFIX			"Ngin3d"

#include "shmacros.h"

static shNgin3dAction		_Ngin3d_actions_kb[350]		= {NGIN3D_NO_ACTION};	// for every key, do nothing
static shNgin3dAction		_Ngin3d_actions[15]			= {NGIN3D_NO_ACTION, NGIN3D_LOOK_AROUND, NGIN3D_NO_ACTION};	// for every input, do nothing, except for NGIN3D_MOUSE_MOVE

void shNgin3dRegisterAction(shNgin3dInput input, shNgin3dAction action, shNgin3dKey key)
{
	if (input != NGIN3D_NO_INPUT)
	{
		if (action == NGIN3D_LOOK_AROUND || input == NGIN3D_MOUSE_MOVE)
		{
			LOG("NGIN3D_MOUSE_MOVE cannot be reregistered! NGIN3D_LOOK_AROUND cannot be registered to any other input either!");
		}
		else if (input == NGIN3D_KEY)
			_Ngin3d_actions_kb[key] = action;
		else
			_Ngin3d_actions[input] = action;
	}
}

static void _Ngin3d_call_actions(shNgin3dAction a, bool pressed)
{
	switch (a)
	{
		case NGIN3D_MOVE_LEFT:
			shNgin3dMoveLeft(pressed);
			break;
		case NGIN3D_MOVE_RIGHT:
			shNgin3dMoveRight(pressed);
			break;
		case NGIN3D_MOVE_FORWARD:
			shNgin3dMoveForward(pressed);
			break;
		case NGIN3D_MOVE_BACKWARD:
			shNgin3dMoveBackward(pressed);
			break;
		case NGIN3D_JUMP:
			shNgin3dJump(pressed);
			break;
		case NGIN3D_CROUCH:
			shNgin3dCrouch(pressed);
			break;
		case NGIN3D_ROLL_RIGHT:
			shNgin3dRollRight(pressed);
			break;
		case NGIN3D_ROLL_LEFT:
			shNgin3dRollLeft(pressed);
			break;
		case NGIN3D_FLY_UP:
			shNgin3dFlyUp(pressed);
			break;
		case NGIN3D_FLY_DOWN:
			shNgin3dFlyDown(pressed);
			break;
		case NGIN3D_LOOK_AROUND:			// should never happen
		case NGIN3D_NO_ACTION:
		default:
			break;
	}
}

void shNgin3dReceivedInput(shNgin3dInput input, bool pressed)
{
	_Ngin3d_call_actions(_Ngin3d_actions[input], pressed);
}

void shNgin3dReceivedInput(shNgin3dInput input, shNgin3dKey key, bool pressed)
{
	if (input == NGIN3D_MOUSE_MOVE)
	{
		LOG("Function shNgin3dInput should not be called with NGIN3D_MOUSE_MOVE unless dx and dy are provided!");
	}
	else if (input != NGIN3D_KEY)
		shNgin3dReceivedInput(input, pressed);
	else
		_Ngin3d_call_actions(_Ngin3d_actions_kb[key], pressed);
}

void shNgin3dReceivedInput(shNgin3dInput input, int dx, int dy)
{
	if (input != NGIN3D_MOUSE_MOVE)
	{
		LOG("Function shNgin3dInput should not be called with input other than NGIN3D_MOUSE_MOVE when dx and dy are provided!");
	}
	else
		shNgin3dLookAround(dx, dy);
}

void shNgin3dUnregisterInputActions()
{
	memset(_Ngin3d_actions, NGIN3D_NO_ACTION, sizeof(_Ngin3d_actions));
	memset(_Ngin3d_actions_kb, NGIN3D_NO_ACTION, sizeof(_Ngin3d_actions_kb));
	_Ngin3d_actions[NGIN3D_MOUSE_MOVE] = NGIN3D_LOOK_AROUND;
}
