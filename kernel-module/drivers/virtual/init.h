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

#ifndef SKIN_DEV_VIRTUAL_INIT_H
#define SKIN_DEV_VIRTUAL_INIT_H

#define					PROCFS_ENTRY					"skin_virtual_input"

#ifdef __KERNEL__

#define					SKINK_MODULE_NAME				"skin_virtual"

#include <skink.h>

typedef struct layer_data
{
	char			name[SKINK_MAX_NAME_LENGTH + 1];
	skink_sensor_size	sensors_count;
	skink_module_size	modules_count;
	skink_patch_size	patches_count;
	uint32_t		acquisition_rate;
	skink_module_id		module_data_begin;
	skink_patch_id		patch_data_begin;
	uint32_t		extra;
} layer_data;

typedef struct module_data
{
	skink_sensor_size	sensors_count;
} module_data;

typedef struct patch_data
{
	skink_module_size	modules_count;
} patch_data;

int init_data(void);
void free_data(void);

#endif

#endif
