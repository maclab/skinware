/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#include "skin_internal.h"
#include "skin_common.h"
#include "skin_sensor_type.h"
#include "skin_sensor.h"
#include "skin_module.h"
#include "skin_patch.h"
#include "skin_object.h"

void skin_patch_init(skin_patch *patch)
{
	*patch = (skin_patch){
		.id = (skin_patch_id)SKIN_INVALID_ID
	};
}

skin_module *skin_patch_modules(skin_patch *patch, skin_module_size *count)
{
	if (patch->p_object == NULL)
		return NULL;

	if (patch->p_object->p_modules == NULL)
	{
		skin_log_set_error(patch->p_object, "skin_object object not initialized properly!");
		return NULL;
	}

	if (count)
		*count = patch->modules_end - patch->modules_begin;
	return patch->p_object->p_modules + patch->modules_begin;
}

skin_sensor_type *skin_patch_sensor_type(skin_patch *patch)
{
	if (patch->p_object == NULL)
		return NULL;

	if (patch->p_object->p_sensor_types == NULL)
	{
		skin_log_set_error(patch->p_object, "skin_object object not initialized properly!");
		return NULL;
	}
	if (patch->sensor_type_id == (skin_sensor_type_id)SKIN_INVALID_ID)
		return NULL;

	return patch->p_object->p_sensor_types + patch->sensor_type_id;
}

void skin_patch_free(skin_patch *patch)
{
	skin_patch_init(patch);
}

void skin_patch_for_each_sensor(skin_patch *patch, skin_callback_sensor c, void *data)
{
	skin_sensor_id si;
	skin_module_id mi;
	skin_sensor_id sbegin, send;
	skin_module_id mbegin, mend;
	skin_object *so;
	skin_sensor *sensors;
	skin_module *modules;

	so = patch->p_object;
	if (so == NULL)
		return;
	sensors = so->p_sensors;
	modules = so->p_modules;

	mbegin = patch->modules_begin;
	mend = patch->modules_end;
	for (mi = mbegin; mi < mend; ++mi)
	{
		skin_module *module = &modules[mi];

		sbegin = module->sensors_begin;
		send = module->sensors_end;
		for (si = sbegin; si < send; ++si)
			if (c(&sensors[si], data) == SKIN_CALLBACK_STOP)
				return;
	}
}

void skin_patch_for_each_module(skin_patch *patch, skin_callback_module c, void *data)
{
	skin_module_id i;
	skin_module_id begin, end;
	skin_object *so;
	skin_module *modules;

	so = patch->p_object;
	if (so == NULL)
		return;
	modules = so->p_modules;

	begin = patch->modules_begin;
	end = patch->modules_end;
	for (i = begin; i < end; ++i)
		if (c(&modules[i], data) == SKIN_CALLBACK_STOP)
			return;
}
