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

#ifndef SKIN_PATCH_HPP
#define SKIN_PATCH_HPP

#include <skin_patch.h>
#include "skin_types.hpp"
#include "skin_callbacks.hpp"
#include "skin_user.hpp"

class Skin;

class SkinPatch
{
public:
	SkinPatch(): patch(NULL), skin(NULL) {}
	SkinPatch(const SkinPatch &) = default;
	SkinPatch &operator =(const SkinPatch &) = default;

	bool isValid() { return patch != NULL && skin != NULL; }

	SkinPatchId getId() { return patch->id; }

	SkinSensorSize sensorCount() { return skin_patch_sensor_count(patch); }
	SkinModuleSize moduleCount() { return skin_patch_module_count(patch); }

	SkinUser getUser() { return SkinUser(patch->user, skin); }
	Skin &getSkin() { return *skin; }

	void *getUserData() { return patch->user_data; }
	void setUserData(void *d) { patch->user_data = d; }

	int forEachSensor(SkinCallback<SkinSensor> callback);
	int forEachModule(SkinCallback<SkinModule> callback);

	/* internal */
	SkinPatch(struct skin_patch *p, Skin *s): patch(p), skin(s) {}

	skin_patch *patch;
	Skin *skin;
};

#endif
