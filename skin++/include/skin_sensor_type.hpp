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

#ifndef SKIN_SENSOR_TYPE_HPP
#define SKIN_SENSOR_TYPE_HPP

#include <skin_sensor_type.h>
#include "skin_types.hpp"
#include "skin_user.hpp"

class SkinSensorType
{
public:
	SkinSensorType(): type(NULL), skin(NULL) {}
	SkinSensorType(const SkinSensorType &) = default;
	SkinSensorType &operator =(const SkinSensorType &) = default;

	bool isValid() { return type != NULL && skin != NULL; }

	SkinSensorTypeId getId() { return type->id; }
	SkinUser getUser() { return SkinUser(type->user, skin); }
	/* TODO: once implemented in C, change getUser to an iterator based approach and add other iterators too */
	Skin &getSkin() { return *skin; }

	/* internal */
	SkinSensorType(struct skin_sensor_type *t, Skin *s): type(t), skin(s) {}

	struct skin_sensor_type *type;
	Skin *skin;
};

#endif
