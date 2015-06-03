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
#include "Ngin3d_physics.h"
#include "Ngin3d_internal.h"
#include "Ngin3d_input.h"

#define				SHMACROS_LOG_PREFIX			"Ngin3d"

#include "shmacros.h"
#include <math.h>

static void _Ngin3d_dummySetCameraPosition(float p[3]) {}
static void _Ngin3d_dummySetCameraTarget(float p[3]) {}
static void _Ngin3d_dummySetCameraUp(float p[3]) {}

bool				_Ngin3d_initialized			= false;
static Ngin3d_setCameraPosition	_Ngin3d_cameraPositionFunction		= _Ngin3d_dummySetCameraPosition;
static Ngin3d_setCameraTarget	_Ngin3d_cameraTargetFunction		= _Ngin3d_dummySetCameraTarget;
static Ngin3d_setCameraUp	_Ngin3d_cameraUpFunction		= _Ngin3d_dummySetCameraUp;
bool				g_Ngin3d_uprightMode			= true;
static bool			_Ngin3d_upsideDownPossible		= false;
static float			_Ngin3d_latitude			= 0;			// latitude 0 is neither up nor down. Can take values in [-89 89] if upside is not possible and in (-180 180]-{-90, 90} if possible
static float			_Ngin3d_longitude			= 180;			// latitude and longitude are used for upright mode only.
												// longitude 0 is for target at 0, ?, z>0 and 0, ?, z<0 if upsidedown.
												// Looking from above, it increases in counter clockwise direction.
												// Can take values in (-180 180]
static float			_Ngin3d_cameraPosition[3]		= {0, 0, 0};		// _Ngin3d_cameraPosition is shared between both free fly and upright modes
static float			_Ngin3d_cameraTarget[3]			= {0, 0, -1};
static float			_Ngin3d_cameraUp[3]			= {0, 1, 0};		// _Ngin3d_cameraTarget and _Ngin3d_cameraUp are only used in free fly mode. At other times are temporarily used to hold conversion from
												// latitude/longitude to send to _Ngin3d_camera*Functions
shNgin3dPhysicalObject		g_Ngin3d_player;
shNgin3dPlayerState		g_Ngin3d_state;
extern float			g_Ngin3d_settings[100];						// from Ngin3d_actions.cpp


#define PI			3.14159265358979323f
#define TAN_1			0.01745506492821759f
#define SIN_1			0.01745240643728351f
#define EPSILON			1e-10f

///////////////////////////////////////////////////
////////// Initialization and finilizing //////////
///////////////////////////////////////////////////
void shNgin3dInitialize()
{
	if (_Ngin3d_initialized)
	{
		shNgin3dQuit();
		_Ngin3d_cameraPositionFunction = _Ngin3d_dummySetCameraPosition;
		_Ngin3d_cameraTargetFunction = _Ngin3d_dummySetCameraTarget;
		_Ngin3d_cameraUpFunction = _Ngin3d_dummySetCameraUp;
		shNgin3dUnregisterInputActions();
		shNing3dResetActionStates();
		g_Ngin3d_player = shNgin3dPhysicalObject();
		_Ngin3d_cameraPosition[0] = 0;
		_Ngin3d_cameraPosition[1] = 0;
		_Ngin3d_cameraPosition[2] = 0;
		_Ngin3d_cameraTarget[0] = 0;
		_Ngin3d_cameraTarget[1] = 0;
		_Ngin3d_cameraTarget[2] = -1;
		_Ngin3d_cameraUp[0] = 0;
		_Ngin3d_cameraUp[1] = 1;
		_Ngin3d_cameraUp[2] = 0;
	}
	g_Ngin3d_state.reset();
	_Ngin3d_initialized = true;
}

void shNgin3dQuit()
{
	_Ngin3d_initialized = false;
}

///////////////////////////////////////////////////
///////// Interface to manipulate camera //////////
///////////////////////////////////////////////////
Ngin3d_setCameraPosition shNgin3dCameraPositionFunction(Ngin3d_setCameraPosition f)
{
	Ngin3d_setCameraPosition t = _Ngin3d_cameraPositionFunction;
	_Ngin3d_cameraPositionFunction = f;
	return t;
}

Ngin3d_setCameraTarget shNgin3dCameraTargetFunction(Ngin3d_setCameraTarget f)
{
	Ngin3d_setCameraTarget t = _Ngin3d_cameraTargetFunction;
	_Ngin3d_cameraTargetFunction = f;
	return t;
}

Ngin3d_setCameraUp shNgin3dCameraUpFunction(Ngin3d_setCameraUp f)
{
	Ngin3d_setCameraUp t = _Ngin3d_cameraUpFunction;
	_Ngin3d_cameraUpFunction = f;
	return t;
}

///////////////////////////////////////////////////
//////////////////// Movement /////////////////////
///////////////////////////////////////////////////

#define CONVERT_LAT_LONG_TO_POINTS \
	do {\
		float lat = _Ngin3d_latitude*PI/180;\
		float lon = _Ngin3d_longitude*PI/180;\
		float lats = sin(lat), latc = cos(lat);\
		float lons = sin(lon), lonc = cos(lon);\
		float targetDir[3] = {latc*lons, lats, latc*lonc};\
		_Ngin3d_cameraUp[0] = -lats*lons;\
		_Ngin3d_cameraUp[1] = latc;\
		_Ngin3d_cameraUp[2] = -lats*lonc;\
		_Ngin3d_cameraTarget[0] = _Ngin3d_cameraPosition[0]+targetDir[0];\
		_Ngin3d_cameraTarget[1] = _Ngin3d_cameraPosition[1]+targetDir[1];\
		_Ngin3d_cameraTarget[2] = _Ngin3d_cameraPosition[2]+targetDir[2];\
/*		LOG("Converted %f(lat)::%f(long) to (%f, %f, %f)(target), (%f, %f, %f)(up)", lat, lon, targetDir[0], targetDir[1], targetDir[2], _Ngin3d_cameraUp[0], _Ngin3d_cameraUp[1], _Ngin3d_cameraUp[2])*/\
	} while (0)

void shNgin3dFreefly()
{
	g_Ngin3d_uprightMode = false;
	CONVERT_LAT_LONG_TO_POINTS;
	(*_Ngin3d_cameraTargetFunction)(_Ngin3d_cameraTarget);
	(*_Ngin3d_cameraUpFunction)(_Ngin3d_cameraUp);
}

void shNgin3dUpright()
{
	g_Ngin3d_uprightMode = true;
	float targetDir[3] = {_Ngin3d_cameraTarget[0]-_Ngin3d_cameraPosition[0], _Ngin3d_cameraTarget[1]-_Ngin3d_cameraPosition[1], _Ngin3d_cameraTarget[2]-_Ngin3d_cameraPosition[2]};
	if ((_Ngin3d_cameraUp[1] < SIN_1 && targetDir[1] > 0) || (_Ngin3d_cameraUp[1] > -SIN_1 && targetDir[1] < 0))
	{
		// latitude minus 1 degree
		targetDir[0] -= _Ngin3d_cameraUp[0]*TAN_1;
		targetDir[1] -= _Ngin3d_cameraUp[1]*TAN_1;
		targetDir[2] -= _Ngin3d_cameraUp[2]*TAN_1;
		float norm = NORM(targetDir);
		targetDir[0] /= norm;
		targetDir[1] /= norm;
		targetDir[2] /= norm;
	}
	else if ((_Ngin3d_cameraUp[1] < SIN_1 && targetDir[1] < 0) || (_Ngin3d_cameraUp[1] > -SIN_1 && targetDir[1] > 0))
	{
		// latitude plus 1 degree
		targetDir[0] += _Ngin3d_cameraUp[0]*TAN_1;
		targetDir[1] += _Ngin3d_cameraUp[1]*TAN_1;
		targetDir[2] += _Ngin3d_cameraUp[2]*TAN_1;
		float norm = NORM(targetDir);
		targetDir[0] /= norm;
		targetDir[1] /= norm;
		targetDir[2] /= norm;
	}
	float lat = asin(targetDir[1]);
	if (_Ngin3d_cameraUp[1] < 0)
		lat = PI-lat;
	float lon = asin(targetDir[0]/cos(lat));		// cos(lat) can't be zero because targetDir has already been shifted to outside ±90±1
	if ((targetDir[2] < 0 && cos(lat) > 0) || (targetDir[2] > 0 && cos(lat) < 0))		// actually: targetDir[2]/cos(lat) < 0
		lon = PI-lon;
	_Ngin3d_latitude = lat*180/PI;
	_Ngin3d_longitude = lon*180/PI;
}

void shNgin3dUpsideDownPossible()
{
	_Ngin3d_upsideDownPossible = true;
}

void shNgin3dUpsideDownNotPossible()
{
	_Ngin3d_upsideDownPossible = false;
	if (g_Ngin3d_uprightMode)
	{
		if (_Ngin3d_latitude > 90 || _Ngin3d_latitude < -90)
		{
			_Ngin3d_latitude = 180-_Ngin3d_latitude;
			if (_Ngin3d_latitude > 180)		// _Ngin3d_latitude was negative
				_Ngin3d_latitude -= 360;
			_Ngin3d_longitude += 180;
			if (_Ngin3d_longitude > 180)
				_Ngin3d_longitude -= 360;
		}
		CONVERT_LAT_LONG_TO_POINTS;
		(*_Ngin3d_cameraTargetFunction)(_Ngin3d_cameraTarget);
		(*_Ngin3d_cameraUpFunction)(_Ngin3d_cameraUp);
	}
	else
	{
		if (_Ngin3d_cameraUp[1] < 0)
		{
			_Ngin3d_cameraUp[0] = -_Ngin3d_cameraUp[0];
			_Ngin3d_cameraUp[1] = -_Ngin3d_cameraUp[1];
			_Ngin3d_cameraUp[2] = -_Ngin3d_cameraUp[2];
			(*_Ngin3d_cameraUpFunction)(_Ngin3d_cameraUp);
		}
	}
}

void shNgin3dMove(float frontSpeed, float rightSpeed, float upSpeed, unsigned long dt)
{
	// cashed version of conversion of latitude and longitude to points already exist in _Ngin3d_cameraTarget and _Ngin3d_cameraUp
	float targetDir[3] = {_Ngin3d_cameraTarget[0]-_Ngin3d_cameraPosition[0], _Ngin3d_cameraTarget[1]-_Ngin3d_cameraPosition[1], _Ngin3d_cameraTarget[2]-_Ngin3d_cameraPosition[2]};
	float rightDir[3] = {CROSS(targetDir, _Ngin3d_cameraUp)};		// targetDir and _Ngin3d_cameraUp should already be normalized
	float upDir[3] = {0, 1, 0};
	if (g_Ngin3d_uprightMode)
	{
		if (_Ngin3d_cameraUp[1] < 0)
			upDir[1] = -1;
	}
	else
	{
		upDir[0] = _Ngin3d_cameraUp[0];
		upDir[1] = _Ngin3d_cameraUp[1];
		upDir[2] = _Ngin3d_cameraUp[2];
	}
	float speed[3] = {frontSpeed*targetDir[0]+upSpeed*upDir[0]+rightSpeed*rightDir[0],
			frontSpeed*targetDir[1]+upSpeed*upDir[1]+rightSpeed*rightDir[1],
			frontSpeed*targetDir[2]+upSpeed*upDir[2]+rightSpeed*rightDir[2]};
	float gravity[3] = {g_Ngin3d_settings[NGIN3D_GRAVITY_X], g_Ngin3d_settings[NGIN3D_GRAVITY_Y], g_Ngin3d_settings[NGIN3D_GRAVITY_Z]};
	float normGravity = NORM(gravity);
	if (g_Ngin3d_uprightMode && !STATE_IN_AIR && normGravity > EPSILON)
	{
		float speedInDirectionOfGravity = DOT(speed, gravity)/normGravity/normGravity;
		speed[0] -= gravity[0]*speedInDirectionOfGravity;
		speed[1] -= gravity[1]*speedInDirectionOfGravity;
		speed[2] -= gravity[2]*speedInDirectionOfGravity;
	}
	shNgin3dCollisionResult cr = g_Ngin3d_player.shNgin3dPOUpdate(dt, speed);
	if (cr == NGIN3D_COLLISION_WITH_GROUND)
	{
		STATE_IN_AIR = false;
	}
	float cameraDisplacement[3] = {0, 0, 0};
	if (g_Ngin3d_uprightMode)
	{
		if (STATE_CROUCHED)
			cameraDisplacement[1] = g_Ngin3d_settings[NGIN3D_CROUCHED_HEIGHT];
		else
			cameraDisplacement[1] = g_Ngin3d_settings[NGIN3D_HEIGHT];
	}
	_Ngin3d_cameraPosition[0] = g_Ngin3d_player.position[0]+cameraDisplacement[0];
	_Ngin3d_cameraPosition[1] = g_Ngin3d_player.position[1]+cameraDisplacement[1];
	_Ngin3d_cameraPosition[2] = g_Ngin3d_player.position[2]+cameraDisplacement[2];
	_Ngin3d_cameraTarget[0] = _Ngin3d_cameraPosition[0]+targetDir[0];
	_Ngin3d_cameraTarget[1] = _Ngin3d_cameraPosition[1]+targetDir[1];
	_Ngin3d_cameraTarget[2] = _Ngin3d_cameraPosition[2]+targetDir[2];
	(*_Ngin3d_cameraPositionFunction)(_Ngin3d_cameraPosition);
	(*_Ngin3d_cameraTargetFunction)(_Ngin3d_cameraTarget);
}

void shNgin3dTwist(float x, float y)
{
	if (g_Ngin3d_uprightMode)
	{
		_Ngin3d_latitude += y;
		if (!_Ngin3d_upsideDownPossible)
		{
			if (_Ngin3d_latitude > 89)
				_Ngin3d_latitude = 89;
			if (_Ngin3d_latitude < -89)
				_Ngin3d_latitude = -89;
		}
		else
		{
			if (_Ngin3d_latitude > 180)
				_Ngin3d_latitude -= 360;
			if (_Ngin3d_latitude <= -180)
				_Ngin3d_latitude += 360;
/*			if (_Ngin3d_latitude <= 90 && _Ngin3d_latitude > 89)
				_Ngin3d_latitude = 89;
			else if (_Ngin3d_latitude > 90 && _Ngin3d_latitude < 91)
				_Ngin3d_latitude = 91;
			else if (_Ngin3d_latitude >= -90 && _Ngin3d_latitude < -89)
				_Ngin3d_latitude = -89;
			else if (_Ngin3d_latitude < -90 && _Ngin3d_latitude > -91)
				_Ngin3d_latitude = -91;*/
		}
			_Ngin3d_longitude -= x;
		if (_Ngin3d_longitude > 180)
			_Ngin3d_longitude -= 360;
		if (_Ngin3d_longitude <= -180)
			_Ngin3d_longitude += 360;
		CONVERT_LAT_LONG_TO_POINTS;
	}
	else
	{
		float targetDir[3] = {_Ngin3d_cameraTarget[0]-_Ngin3d_cameraPosition[0], _Ngin3d_cameraTarget[1]-_Ngin3d_cameraPosition[1], _Ngin3d_cameraTarget[2]-_Ngin3d_cameraPosition[2]};
		float right[3] = {CROSS(targetDir, _Ngin3d_cameraUp)};		// targetDir and _Ngin3d_cameraUp should already be normalized
		targetDir[0] += x*right[0]+y*_Ngin3d_cameraUp[0];
		targetDir[1] += x*right[1]+y*_Ngin3d_cameraUp[1];
		targetDir[2] += x*right[2]+y*_Ngin3d_cameraUp[2];
		float norm = NORM(targetDir);
		targetDir[0] /= norm;
		targetDir[1] /= norm;
		targetDir[2] /= norm;
		float newRight[3] = {CROSS(targetDir, _Ngin3d_cameraUp)};
		if (_Ngin3d_upsideDownPossible || CROSS_Y(newRight, targetDir) > SIN_1)	// if upside down is not allowed and new up is upside down, don't allow the operation
		{
			_Ngin3d_cameraTarget[0] = _Ngin3d_cameraPosition[0]+targetDir[0];
			_Ngin3d_cameraTarget[1] = _Ngin3d_cameraPosition[1]+targetDir[1];
			_Ngin3d_cameraTarget[2] = _Ngin3d_cameraPosition[2]+targetDir[2];
			_Ngin3d_cameraUp[0] = CROSS_X(newRight, targetDir);
			_Ngin3d_cameraUp[1] = CROSS_Y(newRight, targetDir);
			_Ngin3d_cameraUp[2] = CROSS_Z(newRight, targetDir);
			norm = NORM(_Ngin3d_cameraUp);					// to avoid error from cumulating
			_Ngin3d_cameraUp[0] /= norm;
			_Ngin3d_cameraUp[1] /= norm;
			_Ngin3d_cameraUp[2] /= norm;
		}
		else									// move only in the x direction
		{
			targetDir[0] = _Ngin3d_cameraTarget[0]-_Ngin3d_cameraPosition[0]+x*right[0];
			targetDir[1] = _Ngin3d_cameraTarget[1]-_Ngin3d_cameraPosition[1]+x*right[1];
			targetDir[2] = _Ngin3d_cameraTarget[2]-_Ngin3d_cameraPosition[2]+x*right[2];
			float norm = NORM(targetDir);
			targetDir[0] /= norm;
			targetDir[1] /= norm;
			targetDir[2] /= norm;
			newRight[0] = CROSS_X(targetDir, _Ngin3d_cameraUp);
			newRight[1] = CROSS_Y(targetDir, _Ngin3d_cameraUp);
			newRight[2] = CROSS_Z(targetDir, _Ngin3d_cameraUp);
			if (!_Ngin3d_upsideDownPossible && CROSS_Y(newRight, targetDir) < SIN_1)
			{
				LOG("How did this happen?");
			}
			_Ngin3d_cameraTarget[0] = _Ngin3d_cameraPosition[0]+targetDir[0];
			_Ngin3d_cameraTarget[1] = _Ngin3d_cameraPosition[1]+targetDir[1];
			_Ngin3d_cameraTarget[2] = _Ngin3d_cameraPosition[2]+targetDir[2];
			_Ngin3d_cameraUp[0] = CROSS_X(newRight, targetDir);
			_Ngin3d_cameraUp[1] = CROSS_Y(newRight, targetDir);
			_Ngin3d_cameraUp[2] = CROSS_Z(newRight, targetDir);
			norm = NORM(_Ngin3d_cameraUp);					// to avoid error from cumulating
			_Ngin3d_cameraUp[0] /= norm;
			_Ngin3d_cameraUp[1] /= norm;
			_Ngin3d_cameraUp[2] /= norm;
		}
	}
	(*_Ngin3d_cameraTargetFunction)(_Ngin3d_cameraTarget);
	(*_Ngin3d_cameraUpFunction)(_Ngin3d_cameraUp);
}

void shNgin3dRoll(float deg)
{
	if (!g_Ngin3d_uprightMode)
	{
		float targetDir[3] = {_Ngin3d_cameraTarget[0]-_Ngin3d_cameraPosition[0], _Ngin3d_cameraTarget[1]-_Ngin3d_cameraPosition[1], _Ngin3d_cameraTarget[2]-_Ngin3d_cameraPosition[2]};
		float sdeg = sin(deg*PI/180);
		float cdeg = cos(deg*PI/180);
		float rotMat[3][3] =	{{cdeg+targetDir[0]*targetDir[0]*(1-cdeg),		targetDir[0]*targetDir[1]*(1-cdeg)-targetDir[2]*sdeg,	targetDir[0]*targetDir[2]*(1-cdeg)+targetDir[1]*sdeg},
					{targetDir[1]*targetDir[0]*(1-cdeg)+targetDir[2]*sdeg,	cdeg+targetDir[1]*targetDir[1]*(1-cdeg),		targetDir[1]*targetDir[2]*(1-cdeg)-targetDir[0]*sdeg},
					{targetDir[2]*targetDir[0]*(1-cdeg)-targetDir[1]*sdeg,	targetDir[2]*targetDir[1]*(1-cdeg)+targetDir[0]*sdeg,	cdeg+targetDir[2]*targetDir[2]*(1-cdeg)}};
		float newUp[3];
		newUp[1] = DOT(rotMat[1], _Ngin3d_cameraUp);
		if (_Ngin3d_upsideDownPossible || newUp[1] > 0)
		{
			newUp[0] = DOT(rotMat[0], _Ngin3d_cameraUp);
			newUp[2] = DOT(rotMat[2], _Ngin3d_cameraUp);
			float norm = NORM(newUp);
			_Ngin3d_cameraUp[0] = newUp[0]/norm;
			_Ngin3d_cameraUp[1] = newUp[1]/norm;
			_Ngin3d_cameraUp[2] = newUp[2]/norm;
		}
	}
	(*_Ngin3d_cameraUpFunction)(_Ngin3d_cameraUp);
}

void shNgin3dPlacePlayer(float position[3])
{
	g_Ngin3d_player.position[0] = position[0];
	g_Ngin3d_player.position[1] = position[1];
	g_Ngin3d_player.position[2] = position[2];
}

void shNgin3dGetCamera(float position[3], float target[3], float up[3])
{
	position[0] = _Ngin3d_cameraPosition[0];
	position[1] = _Ngin3d_cameraPosition[1];
	position[2] = _Ngin3d_cameraPosition[2];
	if (g_Ngin3d_uprightMode)
		CONVERT_LAT_LONG_TO_POINTS;
	target[0] = _Ngin3d_cameraTarget[0];
	target[1] = _Ngin3d_cameraTarget[1];
	target[2] = _Ngin3d_cameraTarget[2];
	up[0] = _Ngin3d_cameraUp[0];
	up[1] = _Ngin3d_cameraUp[1];
	up[2] = _Ngin3d_cameraUp[2];
}
