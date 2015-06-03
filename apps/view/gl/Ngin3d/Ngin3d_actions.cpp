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

#include "Ngin3d_base.h"
#include "Ngin3d_actions.h"
#include "Ngin3d_physics.h"
#include "Ngin3d_internal.h"
#include <string.h>

#define				SHMACROS_LOG_PREFIX			"Ngin3d"

#include "shmacros.h"
#include <fstream>

using namespace std;

float				g_Ngin3d_settings[100]			= {0};
static bool			_Ngin3d_action_in_progress[20]		= {0};		// for NGIN3D_NO_ACTION and NGIN3D_LOOK_AROUND must always be 0, so they are not even checked
static int			_Ngin3d_mouse_dx			= 0,
				_Ngin3d_mouse_dy			= 0;
extern bool			g_Ngin3d_uprightMode;					// from Ngin3d.cpp
extern shNgin3dPhysicalObject	g_Ngin3d_player;					// from Ngin3d.cpp

void shNgin3dReadSettings(const char *file)
{
	// Set defaults
	shNgin3dSetSetting(NGIN3D_FORWARD_SPEED, 50);
	shNgin3dSetSetting(NGIN3D_BACKWARD_SPEED, 50);
	shNgin3dSetSetting(NGIN3D_STRAFE_SPEED, 30);
	shNgin3dSetSetting(NGIN3D_ASCEND_SPEED, 15);
	shNgin3dSetSetting(NGIN3D_DESCEND_SPEED, 15);
	shNgin3dSetSetting(NGIN3D_CROUCH_SPEED_FACTOR, 0.7);
	shNgin3dSetSetting(NGIN3D_IN_AIR_SPEED_FACTOR, 1);
	shNgin3dSetSetting(NGIN3D_JUMP_INITIAL_SPEED, 250);
	shNgin3dSetSetting(NGIN3D_ROLL_SPEED, 45);
	shNgin3dSetSetting(NGIN3D_YAW_SPEED, 6);
	shNgin3dSetSetting(NGIN3D_PITCH_SPEED, 6);
	shNgin3dSetSetting(NGIN3D_HEIGHT, 0);
	shNgin3dSetSetting(NGIN3D_CROUCHED_HEIGHT, 0);
	shNgin3dSetSetting(NGIN3D_GRAVITY_X, 0);
	shNgin3dSetSetting(NGIN3D_GRAVITY_Y, 0);
	shNgin3dSetSetting(NGIN3D_GRAVITY_Z, 0);
	ifstream fin(file);
	if (!fin.is_open())
	{
		LOG("Could not open settings file %s. Setting values to default", file);
		return;
	}
	string s;
	float v;
#define IF_THEN_SET(x) if (s == #x) shNgin3dSetSetting(NGIN3D_##x, v)
	while (fin >> s >> v)
		IF_THEN_SET(FORWARD_SPEED);
		else IF_THEN_SET(BACKWARD_SPEED);
		else IF_THEN_SET(STRAFE_SPEED);
		else IF_THEN_SET(ASCEND_SPEED);
		else IF_THEN_SET(DESCEND_SPEED);
		else IF_THEN_SET(CROUCH_SPEED_FACTOR);
		else IF_THEN_SET(IN_AIR_SPEED_FACTOR);
		else IF_THEN_SET(JUMP_INITIAL_SPEED);
		else IF_THEN_SET(ROLL_SPEED);
		else IF_THEN_SET(YAW_SPEED);
		else IF_THEN_SET(PITCH_SPEED);
		else IF_THEN_SET(HEIGHT);
		else IF_THEN_SET(CROUCHED_HEIGHT);
		else IF_THEN_SET(GRAVITY_X);
		else IF_THEN_SET(GRAVITY_Y);
		else IF_THEN_SET(GRAVITY_Z);
		else
		{
			LOG("Invalid setting %s in setttings file... ignoring", s.c_str());
		}
}

void shNgin3dSetSetting(shNgin3dSetting s, float v)
{
	g_Ngin3d_settings[s] = v;
	if (s == NGIN3D_GRAVITY_X)
		g_Ngin3d_player.acceleration[0] = v;
	else if (s == NGIN3D_GRAVITY_Y)
		g_Ngin3d_player.acceleration[1] = v;
	else if (s == NGIN3D_GRAVITY_Z)
		g_Ngin3d_player.acceleration[2] = v;
}

void shNgin3dEffectuate(unsigned int dt)
{
	float forwardSpeed = 0, rightSpeed = 0, upSpeed = 0, rollSpeed = 0;
	if (_Ngin3d_action_in_progress[NGIN3D_MOVE_FORWARD]) forwardSpeed += g_Ngin3d_settings[NGIN3D_FORWARD_SPEED];
	if (_Ngin3d_action_in_progress[NGIN3D_MOVE_BACKWARD]) forwardSpeed -= g_Ngin3d_settings[NGIN3D_BACKWARD_SPEED];
	if (_Ngin3d_action_in_progress[NGIN3D_MOVE_LEFT]) rightSpeed -= g_Ngin3d_settings[NGIN3D_STRAFE_SPEED];
	if (_Ngin3d_action_in_progress[NGIN3D_MOVE_RIGHT]) rightSpeed += g_Ngin3d_settings[NGIN3D_STRAFE_SPEED];
	if (_Ngin3d_action_in_progress[NGIN3D_JUMP])
	{
		if (!STATE_IN_AIR && !STATE_CROUCHED)
		{
			g_Ngin3d_player.speed[1] += g_Ngin3d_settings[NGIN3D_JUMP_INITIAL_SPEED];
			STATE_IN_AIR = true;
		}
		// TODO: if there was gravity, check if possible to jump (there is ground beneath) and then set the initial velocity to +NGIN3D_JUMP_INITIAL_SPEED upwards, if not, this key does nothing.
		// Also, check if he's still in the air. Then maybe even second jump allowable.
	}
	if (_Ngin3d_action_in_progress[NGIN3D_CROUCH])
	{
		STATE_CROUCHED = true;
		// TODO: if there was gravity, crouch, but there is no down speed. With crouch, the position of the character doesn't change, but the camera view is lowered
		// Would it be possible to crouch in air like in halflife?
	}
	else
		STATE_CROUCHED = false;
	if (STATE_IN_AIR)
	{
		forwardSpeed *= g_Ngin3d_settings[NGIN3D_IN_AIR_SPEED_FACTOR];
		rightSpeed *= g_Ngin3d_settings[NGIN3D_IN_AIR_SPEED_FACTOR];
	}
	if (STATE_CROUCHED)
	{
		forwardSpeed *= g_Ngin3d_settings[NGIN3D_CROUCH_SPEED_FACTOR];
		rightSpeed *= g_Ngin3d_settings[NGIN3D_CROUCH_SPEED_FACTOR];
	}
	if (_Ngin3d_action_in_progress[NGIN3D_ROLL_RIGHT]) rollSpeed += g_Ngin3d_settings[NGIN3D_ROLL_SPEED];
	if (_Ngin3d_action_in_progress[NGIN3D_ROLL_LEFT]) rollSpeed -= g_Ngin3d_settings[NGIN3D_ROLL_SPEED];
	if (_Ngin3d_action_in_progress[NGIN3D_FLY_UP]) upSpeed += g_Ngin3d_settings[NGIN3D_ASCEND_SPEED];;
	if (_Ngin3d_action_in_progress[NGIN3D_FLY_DOWN]) upSpeed -= g_Ngin3d_settings[NGIN3D_DESCEND_SPEED];;
	shNgin3dMove(forwardSpeed, rightSpeed, upSpeed, dt);
	shNgin3dTwist(_Ngin3d_mouse_dx*g_Ngin3d_settings[NGIN3D_YAW_SPEED]*dt/1000.0f, _Ngin3d_mouse_dy*g_Ngin3d_settings[NGIN3D_PITCH_SPEED]*dt/1000.0f);
	if (!g_Ngin3d_uprightMode)
		shNgin3dRoll(rollSpeed*dt/1000.0f);
	_Ngin3d_mouse_dx = 0;
	_Ngin3d_mouse_dy = 0;
	// TODO: is it done?
}

void shNgin3dMoveLeft(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_MOVE_LEFT] = b;
}

void shNgin3dMoveRight(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_MOVE_RIGHT] = b;
}

void shNgin3dMoveForward(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_MOVE_FORWARD] = b;
}

void shNgin3dMoveBackward(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_MOVE_BACKWARD] = b;
}

void shNgin3dJump(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_JUMP] = b;
}

void shNgin3dCrouch(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_CROUCH] = b;
}

void shNgin3dLookAround(int dx, int dy)
{
	_Ngin3d_mouse_dx += dx;
	_Ngin3d_mouse_dy += dy;
}

void shNgin3dRollLeft(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_ROLL_LEFT] = b;
}

void shNgin3dRollRight(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_ROLL_RIGHT] = b;
}

void shNgin3dFlyUp(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_FLY_UP] = b;
}

void shNgin3dFlyDown(bool b)
{
	_Ngin3d_action_in_progress[NGIN3D_FLY_DOWN] = b;
}

void shNing3dResetActionStates()
{
	memset(_Ngin3d_action_in_progress, 0, sizeof(_Ngin3d_action_in_progress));
}
