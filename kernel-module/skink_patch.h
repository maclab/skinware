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

#ifndef SKINK_PATCH_H
#define SKINK_PATCH_H

#include "skink_common.h"

typedef struct skink_patch
{
	skink_patch_id			id;		// Relative to its layer
	skink_sensor_layer_id		layer;		// which layer this patch is in
	skink_module_id			modules_begin;	// [begin begin+count) will show the beginning and end of modules belonging to the patch, in
	skink_module_size		modules_count;	// the modules list of the patch's layer
} skink_patch;

#endif
