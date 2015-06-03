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

#ifndef SKIN_SENSOR_H
#define SKIN_SENSOR_H

#include "skin_types.h"

URT_DECL_BEGIN

struct skin_module;
struct skin_patch;

/*
 * get_response			get response of sensor
 * get_module			get module this sensor belongs to
 * get_patch			get_patch this sensor belongs to
 */
struct skin_sensor
{
	skin_sensor_id		id;		/* index in its user's sensor array */
	skin_sensor_unique_id	uid;		/* unique hardware-derived id of the sensor (unique in its type) */
	skin_sensor_response	response;	/* sensor response */
	skin_module_id		module;		/* index in its user's module array */
	skin_sensor_type_id	type;		/* type of sensor */
	struct skin_user	*user;		/* user it belongs to */

	void			*user_data;	/* arbitrary user data */

	/* internal */
	skin_sensor_id		next_of_type;	/* next sensor index that has the same type as this sensor */
};

URT_INLINE skin_sensor_response skin_sensor_get_response(struct skin_sensor *sensor) { return sensor->response; }
struct skin_module *skin_sensor_get_module(struct skin_sensor *sensor);
struct skin_patch *skin_sensor_get_patch(struct skin_sensor *sensor);

URT_DECL_END

#endif
