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

void skin_module_init(skin_module *module)
{
	*module = (skin_module){
		.id = (skin_module_id)SKIN_INVALID_ID
	};
}

skin_sensor *skin_module_sensors(skin_module *module, skin_sensor_size *count)
{
	if (module->p_object == NULL)
		return NULL;

	if (module->p_object->p_sensors == NULL)
	{
		skin_log_set_error(module->p_object, "skin_object object not initialized properly!");
		return NULL;
	}

	if (count)
		*count = module->sensors_end - module->sensors_begin;
	return module->p_object->p_sensors + module->sensors_begin;
}

skin_patch *skin_module_patch(skin_module *module)
{
	if (module->p_object)
		return &module->p_object->p_patches[module->patch_id];
	return NULL;
}

skin_sensor_type *skin_module_sensor_type(skin_module *module)
{
	if (module->p_object == NULL)
		return NULL;

	if (module->p_object->p_sensor_types == NULL)
	{
		skin_log_set_error(module->p_object, "skin_object object not initialized properly!");
		return NULL;
	}
	if (module->sensor_type_id == (skin_sensor_type_id)SKIN_INVALID_ID)
		return NULL;

	return module->p_object->p_sensor_types + module->sensor_type_id;
}

void skin_module_free(skin_module *module)
{
	skin_module_init(module);
}

void skin_module_for_each_sensor(skin_module *module, skin_callback_sensor c, void *data)
{
	skin_sensor_id i;
	skin_sensor_id begin, end;
	skin_object *so;
	skin_sensor *sensors;

	so = module->p_object;
	if (so == NULL)
		return;
	sensors = so->p_sensors;

	begin = module->sensors_begin;
	end = module->sensors_end;
	for (i = begin; i < end; ++i)
		if (c(&sensors[i], data) == SKIN_CALLBACK_STOP)
			return;
}
