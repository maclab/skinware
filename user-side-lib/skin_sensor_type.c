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
#include "skin_object.h"
#include "skin_sensor.h"
#include "skin_module.h"
#include "skin_patch.h"

void skin_sensor_type_init(skin_sensor_type *st)
{
	*st = (skin_sensor_type){
		.id = (skin_sensor_type_id)SKIN_INVALID_ID
	};
}

skin_sensor *skin_sensor_type_sensors(skin_sensor_type *st, skin_sensor_size *count)
{
	if (st->p_object == NULL)
		return NULL;
	if (st->p_object->p_sensors == NULL)
	{
		skin_log_set_error(st->p_object, "skin_object object not initialized properly!");
		return NULL;
	}
	if (count)
		*count = st->sensors_end - st->sensors_begin;
	return st->p_object->p_sensors + st->sensors_begin;
}

skin_module *skin_sensor_type_modules(skin_sensor_type *st, skin_module_size *count)
{
	if (st->p_object == NULL)
		return NULL;
	if (st->p_object->p_modules == NULL)
	{
		skin_log_set_error(st->p_object, "skin_object object not initialized properly!");
		return NULL;
	}
	if (count)
		*count = st->modules_end - st->modules_begin;
	return st->p_object->p_modules + st->modules_begin;
}

skin_patch *skin_sensor_type_patches(skin_sensor_type *st, skin_patch_size *count)
{
	if (st->p_object == NULL)
		return NULL;
	if (st->p_object->p_patches == NULL)
	{
		skin_log_set_error(st->p_object, "skin_object object not initialized properly!");
		return NULL;
	}
	if (count)
		*count = st->patches_end - st->patches_begin;
	return st->p_object->p_patches + st->patches_begin;
}

void skin_sensor_type_free(skin_sensor_type *st)
{
	skin_sensor_type_init(st);
}

void skin_sensor_type_for_each_sensor(skin_sensor_type *st, skin_callback_sensor c, void *data)
{
	skin_sensor_id i;
	skin_sensor_id begin, end;
	skin_object *so;
	skin_sensor *sensors;

	so = st->p_object;
	if (so == NULL)
		return;
	sensors = so->p_sensors;

	begin = st->sensors_begin;
	end = st->sensors_end;
	for (i = begin; i < end; ++i)
		if (c(&sensors[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_sensor_type_for_each_module(skin_sensor_type *st, skin_callback_module c, void *data)
{
	skin_module_id i;
	skin_module_id begin, end;
	skin_object *so;
	skin_module *modules;

	so = st->p_object;
	if (so == NULL)
		return;
	modules = so->p_modules;

	begin = st->modules_begin;
	end = st->modules_end;
	for (i = begin; i < end; ++i)
		if (c(&modules[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_sensor_type_for_each_patch(skin_sensor_type *st, skin_callback_patch c, void *data)
{
	skin_patch_id i;
	skin_patch_id begin, end;
	skin_object *so;
	skin_patch *patches;

	so = st->p_object;
	if (so == NULL)
		return;
	patches = so->p_patches;

	begin = st->patches_begin;
	end = st->patches_end;
	for (i = begin; i < end; ++i)
		if (c(&patches[i], data) == SKIN_CALLBACK_STOP)
			return;
}
