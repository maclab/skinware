/*
 * Copyright (C) 2011-2015  Maclab, DIBRIS, Universita di Genova <info@cyskin.com>
 * Authored by Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SKIN_USER_HPP
#define SKIN_USER_HPP

#include <functional>
#include <skin_user.h>
#include "skin_types.hpp"
#include "skin_callbacks.hpp"
#include "skin_reader.hpp"

class SkinUser;

class SkinUserAttr
{
public:
	SkinUserAttr(SkinSensorTypeId sensorType)
	{
		attr.sensor_type = sensorType;
	}
	SkinUserAttr(const struct skin_user_attr &a)
	{
		attr = a;
	}

	SkinSensorTypeId getSensorType() { return attr.sensor_type; }

	/* internal */
	struct skin_user_attr attr;
};

class SkinUserCallbacks
{
public:
	typedef std::function<void (SkinUser, SkinSensorResponse *, SkinSensorSize)> peekCallback;
	typedef std::function<void (SkinUser &)> initCallback;
	typedef std::function<void (SkinUser &)> cleanCallback;
	typedef std::function<void (SkinReader &)> readerInitCallback;
	typedef std::function<void (SkinReader &)> readerCleanCallback;
	SkinUserCallbacks(peekCallback pc, initCallback ic = NULL, cleanCallback cc = NULL,
			readerInitCallback ric = NULL, readerCleanCallback rcc = NULL):
		peek(pc), init(ic), clean(cc), readerInit(ric), readerClean(rcc) {}

	/* internal */
	peekCallback peek;
	initCallback init;
	cleanCallback clean;
	readerInitCallback readerInit;
	readerCleanCallback readerClean;
};

class Skin;
class SkinReader;
class SkinSensor;
class SkinModule;
class SkinPatch;
class SkinSensorType;

class SkinUser
{
public:
	SkinUser(): user(NULL), skin(NULL) {}
	SkinUser(const SkinUser &) = default;
	SkinUser &operator =(const SkinUser &) = default;

	bool isValid() { return user != NULL && skin != NULL; }

	int pause() { return skin_user_pause(user); }
	int resume() { return skin_user_resume(user); }
	bool isPaused() { return skin_user_is_paused(user); }
	bool isActive() { return skin_user_is_active(user); }

	SkinReader getReader() { return SkinReader(skin_user_get_reader(user), skin); }
	Skin &getSkin() { return *skin; }

	SkinSensorSize sensorCount() { return skin_user_sensor_count(user); }
	SkinModuleSize moduleCount() { return skin_user_module_count(user); }
	SkinPatchSize patchCount() { return skin_user_patch_count(user); }
	SkinSensorTypeSize sensorTypeCount() { return skin_user_sensor_type_count(user); }

	int forEachSensor(SkinCallback<SkinSensor> callback);
	int forEachModule(SkinCallback<SkinModule> callback);
	int forEachPatch(SkinCallback<SkinPatch> callback);
	int forEachSensorType(SkinCallback<SkinSensorType> callback);
	int forEachSensorOfType(SkinSensorTypeId type, SkinCallback<SkinSensor> callback);

	/* internal */
	SkinUser(struct skin_user *u, Skin *s): user(u), skin(s) {}

	struct skin_user *user;
	Skin *skin;
};

#endif
