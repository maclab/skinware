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

#ifndef SKINK_REGION_H
#define SKINK_REGION_H

#include "skink_common.h"

typedef struct skink_region
{
	skink_region_id			id;
	skink_sub_region_index_id	sub_region_indices_begin;	// These are indices to an array of sub-region indices that when indexed
	skink_sub_region_index_size	sub_region_indices_count;	// in [sub_regions_begin sub_regions_begin+sub_regions_count) will give indices
									// to sub-regions that are included in this region
} skink_region;

#endif
