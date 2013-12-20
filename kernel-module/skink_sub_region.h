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

#ifndef SKINK_SUB_REGION_H
#define SKINK_SUB_REGION_H

#include "skink_common.h"

typedef struct skink_sub_region
{
	skink_sub_region_id		id;
	skink_region_index_id		region_indices_begin;		// similar to skink_region's sub_region_indices_begin and sub_region_indices_count
	skink_region_index_size		region_indices_count;		// these values are indices to an array of region indices that when indexed
									// in [regions_begin regions_begin+regions_count) will give indices to regions that include
									// this sub-region
	struct skink_sr_layer
	{
		skink_sensor_id		sensors_begin;			// Each sub-region possibly has sensors in each layer.  [sensors_begin sensors_begin+sensors_count)
		skink_sensor_size	sensors_count;			// of layers[i] are indices to
									// ((skink_sensor *)the_skink_layers[i]).sensors
	} layers[SKINK_MAX_SENSOR_LAYERS];
} skink_sub_region;

#endif
