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

#ifndef SKIN_SENSOR_HPP
#define SKIN_SENSOR_HPP

#include <functional>
#include <skin_sensor.h>
#include "skin_types.hpp"
#include "skin_module.hpp"
#include "skin_patch.hpp"
#include "skin_user.hpp"

class Skin;

class SkinSensor
{
public:
	SkinSensor(): sensor(NULL), skin(NULL) {}
	SkinSensor(const SkinSensor &) = default;
	SkinSensor &operator =(const SkinSensor &) = default;

	bool isValid() { return sensor != NULL && skin != NULL; }

	SkinSensorId getId() { return sensor->id; }
	SkinSensorUniqueId getUid() { return sensor->uid; }
	SkinSensorResponse getResponse() { return skin_sensor_get_response(sensor); }
	SkinSensorTypeId getType() { return sensor->type; }

	SkinModule getModule() { return SkinModule(skin_sensor_get_module(sensor), skin); }
	SkinPatch getPatch() { return SkinPatch(skin_sensor_get_patch(sensor), skin); }
	SkinUser getUser() { return SkinUser(sensor->user, skin); }
	Skin &getSkin() { return *skin; }

	void *getUserData() { return sensor->user_data; }
	void setUserData(void *d) { sensor->user_data = d; }

	/* internal */
	SkinSensor(struct skin_sensor *n, Skin *s): sensor(n), skin(s) {}

	skin_sensor *sensor;
	Skin *skin;
};

#endif
