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

#define URT_LOG_PREFIX "skin: "
#include <skin_patch.h>
#include <skin_module.h>
#include "user_internal.h"

static int _count_sensors(struct skin_module *m, void *d)
{
	*(skin_sensor_size *)d += m->sensor_count;
	return SKIN_CALLBACK_CONTINUE;
}

skin_sensor_size skin_patch_sensor_count(struct skin_patch *p)
{
	skin_sensor_size res = 0;
	skin_patch_for_each_module(p, _count_sensors, &res);
	return res;
}
URT_EXPORT_SYMBOL(skin_patch_sensor_count);
