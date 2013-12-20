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
#include "skin_sensor.h"
#include "skin_sensor_type.h"
#include "skin_sub_region.h"
#include "skin_object.h"
#include "skin_module.h"
#include "skin_patch.h"
#include "skin_region.h"

void skin_sensor_init(skin_sensor *sensor)
{
	*sensor = (skin_sensor){
		.id = (skin_sensor_id)SKIN_INVALID_ID
	};
}

skin_sensor_type *skin_sensor_sensor_type(skin_sensor *sensor)
{
	if (sensor->p_object)
		return &sensor->p_object->p_sensor_types[sensor->sensor_type_id];
	return NULL;
}

skin_sub_region *skin_sensor_sub_region(skin_sensor *sensor)
{
	if (sensor->p_object)
		return &sensor->p_object->p_sub_regions[sensor->sub_region_id];
	return NULL;
}

skin_module *skin_sensor_module(skin_sensor *sensor)
{
	if (sensor->p_object)
		return &sensor->p_object->p_modules[sensor->module_id];
	return NULL;
}

skin_patch *skin_sensor_patch(skin_sensor *sensor)
{
	if (sensor->p_object)
		return &sensor->p_object->p_patches[sensor->p_object->p_modules[sensor->module_id].patch_id];
	return NULL;
}

void skin_sensor_free(skin_sensor *sensor)
{
	skin_sensor_init(sensor);
}

void skin_sensor_for_each_region(skin_sensor *sensor, skin_callback_region c, void *data)
{
	skin_region_index_id i;
	skin_region_id begin, end;
	skin_object *so;
	skin_sub_region *sub_regions;
	skin_region *regions;
	skin_region_id *region_indices;

	so = sensor->p_object;
	if (so == NULL)
		return;
	sub_regions = so->p_sub_regions;
	regions = so->p_regions;
	region_indices = so->p_region_indices;

	begin = sub_regions[sensor->sub_region_id].region_indices_begin;
	end = sub_regions[sensor->sub_region_id].region_indices_end;
	for (i = begin; i < end; ++i)
		if (c(&regions[region_indices[i]], data) == SKIN_CALLBACK_STOP)
			return;
}
