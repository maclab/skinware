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

void skin_region_init(skin_region *region)
{
	*region = (skin_region){
		.id = (skin_region_id)SKIN_INVALID_ID
	};
}

skin_sub_region_id *skin_region_sub_region_indices(skin_region *region, skin_sub_region_index_size *count)
{
	if (region->p_object == NULL)
		return NULL;

	if (region->p_object->p_sub_region_indices == NULL)
	{
		skin_log_set_error(region->p_object, "skin_object object not initialized properly!");
		return NULL;
	}
	if (count)
		*count = region->sub_region_indices_end - region->sub_region_indices_begin;
	return region->p_object->p_sub_region_indices + region->sub_region_indices_begin;
}

void skin_region_free(skin_region *region)
{
	skin_region_init(region);
}

void skin_region_for_each_sensor(skin_region *region, skin_callback_sensor c, void *data)
{
	skin_sensor_id si;
	skin_sub_region_id sri;
	skin_sensor_type_id sti;
	skin_sensor_id sbegin, send;
	skin_sub_region_id srbegin, srend;
	skin_sensor_type_id stbegin, stend;
	skin_object *so;
	skin_sensor *sensors;
	skin_sub_region *sub_regions;
	skin_sub_region_id *sub_region_indices;

	so = region->p_object;
	if (so == NULL)
		return;
	sensors = so->p_sensors;
	sub_regions = so->p_sub_regions;
	sub_region_indices = so->p_sub_region_indices;

	stbegin = 0;
	stend = so->p_sensor_types_count;
	srbegin = region->sub_region_indices_begin;
	srend = region->sub_region_indices_end;
	for (sti = stbegin; sti < stend; ++sti)
		for (sri = srbegin; sri < srend; ++sri)
		{
			skin_sub_region *sr = &sub_regions[sub_region_indices[sri]];

			sbegin = sr->sensors_begins[sti];
			send = sr->sensors_ends[sti];
			for (si = sbegin; si < send; ++si)
				if (c(&sensors[si], data) == SKIN_CALLBACK_STOP)
					return;
		}
}

void skin_region_for_each_sub_region(skin_region *region, skin_callback_sub_region c, void *data)
{
	skin_sub_region_index_id i;
	skin_sub_region_index_id begin, end;
	skin_object *so;
	skin_sub_region *sub_regions;
	skin_sub_region_id *sub_region_indices;

	so = region->p_object;
	if (so == NULL)
		return;
	sub_regions = so->p_sub_regions;
	sub_region_indices = so->p_sub_region_indices;

	begin = region->sub_region_indices_begin;
	end = region->sub_region_indices_end;
	for (i = begin; i < end; ++i)
		if (c(&sub_regions[sub_region_indices[i]], data) == SKIN_CALLBACK_STOP)
			return;
}
