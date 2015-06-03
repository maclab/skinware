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

#ifndef SKIN_MODULE_H
#define SKIN_MODULE_H

#include "skin_types.h"
#include "skin_callbacks.h"

URT_DECL_BEGIN

struct skin_sensor;
struct skin_patch;

/*
 * sensor_count			number of sensors
 * get_patch			get patch this module belongs to
 * for_each_sensor		iterators over sensors.  The return value is 0 if all were
 *				iterated and non-zero if the callback had terminated the iteration prematurely,
 *				or if there was an error.
 */
struct skin_module
{
	skin_module_id		id;		/* index in its user's module array */
	skin_patch_id		patch;		/* index in its user's patch array */
	struct skin_user	*user;		/* user it belongs to */
	struct skin_sensor	*sensors;	/* a pointer to the module sensor data */
	skin_sensor_size	sensor_count;	/* number of elements in sensors array */

	void			*user_data;	/* arbitrary user data */
};

URT_INLINE skin_sensor_size skin_module_sensor_count(struct skin_module *module) { return module->sensor_count; }
struct skin_patch *skin_module_get_patch(struct skin_module *module);

#define skin_module_for_each_sensor(...) skin_module_for_each_sensor(__VA_ARGS__, NULL)
int (skin_module_for_each_sensor)(struct skin_module *module, skin_callback_sensor callback, void *user_data, ...);

URT_DECL_END

#endif
