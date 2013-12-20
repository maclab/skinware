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
#include "skin_sub_region.h"
#include "skin_region.h"
#include "skin_object.h"

void skin_sub_region_init(skin_sub_region *sub_region)
{
	*sub_region = (skin_sub_region){
		.id = (skin_sub_region_id)SKIN_INVALID_ID
	};
}

skin_sensor_type_size skin_sub_region_sensor_types_count(skin_sub_region *sub_region)
{
	if (sub_region->p_object == NULL)
		return SKIN_FAIL;
	return sub_region->p_object->p_sensor_types_count;
}

skin_sensor_type_size skin_sub_region_sensors(skin_sub_region *sub_region, skin_sensor **sensors, skin_sensor_size *count)
{
	skin_sensor_type_id i;
	skin_sensor_type_size types;

	if (sub_region->p_object == NULL || sub_region->sensors_begins == NULL || sub_region->sensors_ends == NULL)
		return SKIN_FAIL;

	if (sensors == NULL)
	{
		skin_log_set_error(sub_region->p_object, "parameter to sensors is null");
		return SKIN_FAIL;
	}
	if (sub_region->p_object->p_sensors == NULL)
	{
		skin_log_set_error(sub_region->p_object, "skin_object object not initialized properly!");
		return SKIN_FAIL;
	}

	types = sub_region->p_object->p_sensor_types_count;
	if (count)
		for (i = 0; i < types; ++i)
			count[i] = sub_region->sensors_ends[i] - sub_region->sensors_begins[i];
	for (i = 0; i < types; ++i)
		sensors[i] = sub_region->p_object->p_sensors + sub_region->sensors_begins[i];
	return types;
}

skin_region_id *skin_sub_region_region_indices(skin_sub_region *sub_region, skin_region_index_size *count)
{
	if (sub_region->p_object == NULL)
		return NULL;

	if (sub_region->p_object->p_region_indices == NULL)
	{
		skin_log_set_error(sub_region->p_object, "skin_object object not initialized properly!");
		return NULL;
	}

	if (count)
		*count = sub_region->region_indices_end - sub_region->region_indices_begin;
	return sub_region->p_object->p_region_indices + sub_region->region_indices_begin;
}

void skin_sub_region_free(skin_sub_region *sub_region)
{
	skin_sub_region_init(sub_region);
}

void skin_sub_region_for_each_sensor(skin_sub_region *sub_region, skin_callback_sensor c, void *data)
{
	skin_sensor_id si;
	skin_sensor_type_id sti;
	skin_sensor_id sbegin, send;
	skin_sensor_type_id stbegin, stend;
	skin_object *so;
	skin_sensor *sensors;
	skin_sensor_id *sensors_begins;
	skin_sensor_id *sensors_ends;

	so = sub_region->p_object;
	if (so == NULL)
		return;
	sensors = so->p_sensors;
	sensors_begins = sub_region->sensors_begins;
	sensors_ends = sub_region->sensors_ends;

	stbegin = 0;
	stend = so->p_sensor_types_count;
	for (sti = stbegin; sti < stend; ++sti)
	{
		sbegin = sensors_begins[sti];
		send = sensors_ends[sti];
		for (si = sbegin; si < send; ++si)
			if (c(&sensors[si], data) == SKIN_CALLBACK_STOP)
				return;
	}
}

void skin_sub_region_for_each_region(skin_sub_region *sub_region, skin_callback_region c, void *data)
{
	skin_region_index_id i;
	skin_region_index_id begin, end;
	skin_object *so;
	skin_region *regions;
	skin_region_id *region_indices;

	so = sub_region->p_object;
	if (so == NULL)
		return;
	regions = so->p_regions;
	region_indices = so->p_region_indices;

	begin = sub_region->region_indices_begin;
	end = sub_region->region_indices_end;
	for (i = begin; i < end; ++i)
		if (c(&regions[region_indices[i]], data) == SKIN_CALLBACK_STOP)
			return;
}
