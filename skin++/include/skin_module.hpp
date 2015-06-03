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

#ifndef SKIN_MODULE_HPP
#define SKIN_MODULE_HPP

#include <skin_module.h>
#include "skin_types.hpp"
#include "skin_callbacks.hpp"
#include "skin_patch.hpp"
#include "skin_user.hpp"

class Skin;

class SkinModule
{
public:
	SkinModule(): module(NULL), skin(NULL) {}
	SkinModule(const SkinModule &) = default;
	SkinModule &operator =(const SkinModule &) = default;

	bool isValid() { return module != NULL && skin != NULL; }

	SkinModuleId getId() { return module->id; }

	SkinSensorSize sensorCount() { return skin_module_sensor_count(module); }

	SkinPatch getPatch() { return SkinPatch(skin_module_get_patch(module), skin); }
	SkinUser getUser() { return SkinUser(module->user, skin); }
	Skin &getSkin() { return *skin; }

	void *getUserData() { return module->user_data; }
	void setUserData(void *d) { module->user_data = d; }

	int forEachSensor(SkinCallback<SkinSensor> callback);

	/* internal */
	SkinModule(struct skin_module *m, Skin *s): module(m), skin(s) {}

	skin_module *module;
	Skin *skin;
};

#endif
