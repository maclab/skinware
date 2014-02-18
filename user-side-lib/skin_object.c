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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "skin_internal.h"
#include "skin_common.h"
#include "skin_object.h"
#include "skin_sensor.h"
#include "skin_sub_region.h"
#include "skin_region.h"
#include "skin_module.h"
#include "skin_patch.h"
#include "skin_sensor_type.h"

#define SKIN_LOAD_FROM_INITIALIZED_KERNEL	0
#define SKIN_LOAD_FOR_CALIBRATION		1
#define SKIN_LOAD_FOR_REGIONALIZATION		2

#define CLEAN_FREE(x) \
	do {\
		free(x);\
		x = NULL;\
		x##_count = 0;\
	} while (0)

#define SKIN_CHECK_FILE_INTEGRITY(so, f, x)							\
	do {											\
		if (feof(f))									\
		{										\
			skin_log_set_error(so, "File passed as %s file is incomplete!", x);	\
			ret = SKIN_FILE_INCOMPLETE;						\
			goto exit_fail;								\
		}										\
		else if (ferror(f))								\
		{										\
			skin_log_set_error(so, "File passed as %s file is erroneous!", x);	\
			ret = SKIN_FILE_PARSE_ERROR;						\
			goto exit_fail;								\
		}										\
	} while (0)

void skin_object_init(skin_object *so)
{
	*so = (skin_object){0};
#ifndef SKIN_DS_ONLY
	skin_reader_init(&so->p_reader);
	skin_service_manager_init(&so->p_service_manager);
#endif
}

void skin_object_free(skin_object *so)
{
	skin_log("Cleaning up skin...  ");
#ifndef SKIN_DS_ONLY
	skin_reader_internal_terminate(&so->p_reader);
	skin_reader_free(&so->p_reader);
	skin_service_manager_internal_terminate(&so->p_service_manager);
	skin_service_manager_free(&so->p_service_manager);
	free(so->p_error_message);
#endif
	free(so->p_sensor_types);
	free(so->p_sensors);
	free(so->p_modules);
	free(so->p_patches);
	free(so->p_sub_regions);
	free(so->p_regions);
	free(so->p_sub_region_indices);
	free(so->p_region_indices);
	free(so->p_sub_region_sensors_begins);
	free(so->p_sub_region_sensors_ends);
	free(so->p_sensor_neighbors);
	skin_object_init(so);
	skin_log("Cleaning up skin... done");
	skin_log_free();
}

#ifndef SKIN_DS_ONLY
skin_rt_time skin_object_time_diff_with_kernel(skin_object *so)
{
	int i;
	skin_rt_time kernel_time_to_user = 0;
	unsigned int times = 0;
	bool error_given = false;

	if (so->p_time_diff_with_kernel != 0)
		return so->p_time_diff_with_kernel;

	for (i = 0; i < 1000; ++i)
	{
		FILE *tempin;
		skin_rt_time user_time, kernel_time;

		tempin = fopen("/sys/skin_kernel/current_time", "r");
		if (tempin == NULL)
			break;
		if (fscanf(tempin, "%llu", &kernel_time) != 1)
		{
			if (!error_given)
				skin_log("Error reading from /sys/skin_kernel/current_time");
			error_given = true;
			continue;
		}
		fclose(tempin);

		user_time = skin_rt_get_time();

		kernel_time_to_user += user_time - kernel_time;
		++times;
	}
	if (times > 0)
		kernel_time_to_user /= times;
	if (kernel_time_to_user != 0)
		so->p_time_diff_with_kernel = kernel_time_to_user;

	return kernel_time_to_user;
}

#define SKINK_MODULE_LOADED			misc_data[SKINK_MODULE_LOADED_INDEX]
#define SKINK_INITIALIZED			misc_data[SKINK_INITIALIZED_INDEX]
#define SKINK_INITIALIZATION_FAILED		misc_data[SKINK_INITIALIZATION_FAILED_INDEX]
#define SKINK_SENSOR_LAYERS_COUNT		misc_data[SKINK_SENSOR_LAYERS_COUNT_INDEX]
#define SKINK_SENSORS_COUNT			misc_data[SKINK_SENSORS_COUNT_INDEX]
#define SKINK_SUB_REGIONS_COUNT			misc_data[SKINK_SUB_REGIONS_COUNT_INDEX]
#define SKINK_REGIONS_COUNT			misc_data[SKINK_REGIONS_COUNT_INDEX]
#define SKINK_MODULES_COUNT			misc_data[SKINK_MODULES_COUNT_INDEX]
#define SKINK_PATCHES_COUNT			misc_data[SKINK_PATCHES_COUNT_INDEX]
#define SKINK_SUB_REGION_INDICES_COUNT		misc_data[SKINK_SUB_REGION_INDICES_COUNT_INDEX]
#define SKINK_REGION_INDICES_COUNT		misc_data[SKINK_REGION_INDICES_COUNT_INDEX]
#define SKINK_SENSOR_NEIGHBORS_COUNT		misc_data[SKINK_SENSOR_NEIGHBORS_COUNT_INDEX]
#define SKINK_CALIBRATION_POSSIBLE		misc_data[SKINK_CALIBRATION_POSSIBLE_INDEX]
#define SKINK_CALIBRATION_TOO_EARLY		misc_data[SKINK_CALIBRATION_TOO_EARLY_INDEX]
#define SKINK_REGIONALIZATION_POSSIBLE		misc_data[SKINK_REGIONALIZATION_POSSIBLE_INDEX]
#define SKINK_REGIONALIZATION_TOO_EARLY		misc_data[SKINK_REGIONALIZATION_TOO_EARLY_INDEX]
#define SKINK_REGIONALIZATION_SYNC		misc_data[SKINK_REGIONALIZATION_SYNC_INDEX]

#define SKINK_SIZEOF_SENSOR_LAYER		misc_data[SKINK_SIZEOF_SENSOR_LAYER_INDEX]
#define SKINK_SIZEOF_SENSOR			misc_data[SKINK_SIZEOF_SENSOR_INDEX]
#define SKINK_SIZEOF_MODULE			misc_data[SKINK_SIZEOF_MODULE_INDEX]
#define SKINK_SIZEOF_PATCH			misc_data[SKINK_SIZEOF_PATCH_INDEX]
#define SKINK_SIZEOF_SUB_REGION			misc_data[SKINK_SIZEOF_SUB_REGION_INDEX]
#define SKINK_SIZEOF_REGION			misc_data[SKINK_SIZEOF_REGION_INDEX]

static skink_misc_datum *_get_misc_and_check_phase(skin_object *so, int phase)
{
	skink_misc_datum *misc_data = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	if (misc_data == NULL)
	{
		skin_log_set_error(so, "Could not attach to misc shared memory!");
		return NULL;
	}
	if (!SKINK_MODULE_LOADED)
	{
		skin_log_set_error(so, "The skin kernel is unloaded!");
		goto exit_fail;
	}
	if (SKINK_INITIALIZATION_FAILED)
	{
		skin_log_set_error(so, "The skin kernel has failed in initialization!");
		goto exit_fail;
	}
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL && !SKINK_INITIALIZED)
	{
		if (SKINK_CALIBRATION_POSSIBLE)
			skin_log_set_error(so, "The calibration program needs to first be executed.");
		else if (SKINK_REGIONALIZATION_POSSIBLE)
			skin_log_set_error(so, "The regionalization program needs to first be executed.");
		else
			skin_log_set_error(so, "The skin kernel is not yet initialized!");
		goto exit_fail;
	}
	else if (phase == SKIN_LOAD_FOR_CALIBRATION && !SKINK_CALIBRATION_POSSIBLE)
	{
		/* TODO: is this if necessary, or only the else? */
		if (!SKINK_CALIBRATION_TOO_EARLY)
			skin_log_set_error(so, "Skin is already calibrated.");
		else
		{
			skin_log_set_error(so, "The skin kernel is not in calibration phase yet.  Waiting...");
			while (SKINK_CALIBRATION_TOO_EARLY && !SKINK_CALIBRATION_POSSIBLE)
				usleep(1000);
		}
		if (!SKINK_MODULE_LOADED)
		{
			skin_log_set_error(so, "The skin kernel is unloaded");
			goto exit_fail;
		}
		if (SKINK_REGIONALIZATION_POSSIBLE || SKINK_INITIALIZED)
		{
			skin_log_set_error(so, "Skin is already calibrated");
			goto exit_fail;
		}
		if (!SKINK_CALIBRATION_POSSIBLE)
		{
			skin_log_set_error(so, "There are two calibrators running, or someone is messing with the kernel's /proc file");
			goto exit_fail;
		}
	}
	else if (phase == SKIN_LOAD_FOR_REGIONALIZATION && !SKINK_REGIONALIZATION_POSSIBLE)
	{
		/* TODO: same as with calibration */
		if (!SKINK_REGIONALIZATION_TOO_EARLY)
			skin_log_set_error(so, "Skin is already regionalized.");
		else
		{
			skin_log_set_error(so, "The skin kernel is not in regionalization phase yet.  Waiting...");
			while (SKINK_REGIONALIZATION_TOO_EARLY && !SKINK_REGIONALIZATION_POSSIBLE)
				usleep(1000);
		}
		if (!SKINK_MODULE_LOADED)
		{
			skin_log_set_error(so, "The skin kernel is unloaded");
			goto exit_fail;
		}
		if (SKINK_INITIALIZED)
		{
			skin_log_set_error(so, "Skin is already regionalized");
			goto exit_fail;
		}
		if (!SKINK_REGIONALIZATION_POSSIBLE)
		{
			skin_log_set_error(so, "There are two regionalizers running, or someone is messing with the kernel's /proc file");
			goto exit_fail;
		}
	}

	return misc_data;
exit_fail:
	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	return NULL;
}

static bool _compatible(skin_object *so, skink_misc_datum *misc_data)
{
	bool compatible = true;

#define CHECK_SIZEOF(name_c, name_s) \
	do {\
		if (SKINK_SIZEOF_##name_c != sizeof(skink_##name_s))\
		{\
			skin_log_set_error(so, "Mismatch in size of skinkernel"#name_s);\
			compatible = false;\
		}\
	} while (0)
	CHECK_SIZEOF(SENSOR_LAYER, sensor_layer);
	CHECK_SIZEOF(SENSOR, sensor);
	CHECK_SIZEOF(MODULE, module);
	CHECK_SIZEOF(PATCH, patch);
	CHECK_SIZEOF(SUB_REGION, sub_region);
	CHECK_SIZEOF(REGION, region);
#undef CHECK_SIZEOF

	return compatible;
}

static void _copy_sensor_types(skin_object *so, skink_sensor_layer *from)
{
	skin_sensor_type *to = so->p_sensor_types;
	skin_sensor_type_size count = so->p_sensor_types_count;
	skin_sensor_id current_sensor_start = 0;
	skin_module_id current_module_start = 0;
	skin_patch_id current_patch_start = 0;
	skin_sensor_type_id i;
	skin_sensor_id si;
	skin_module_id mi;
	skin_patch_id pi;

	for (i = 0; i < count; ++i)
	{
		to[i].id		= i;
		to[i].name		= strdup(from[i].name);
		to[i].sensors_begin	= current_sensor_start;
		to[i].modules_begin	= current_module_start;
		to[i].patches_begin	= current_patch_start;
		current_sensor_start	+= from[i].sensors_count;
		current_module_start	+= from[i].modules_count;
		current_patch_start	+= from[i].patches_count;
		to[i].sensors_end	= current_sensor_start;
		to[i].modules_end	= current_module_start;
		to[i].patches_end	= current_patch_start;

		/* set references from other structures to this one */
		for (si = to[i].sensors_begin; si < to[i].sensors_end; ++si)
			so->p_sensors[si].sensor_type_id = i;
		for (mi = to[i].modules_begin; mi < to[i].modules_end; ++mi)
			so->p_modules[mi].sensor_type_id = i;
		for (pi = to[i].patches_begin; pi < to[i].patches_end; ++pi)
			so->p_patches[pi].sensor_type_id = i;
	}
}

static void _copy_patches(skin_object *so, skink_patch *from)
{
	skin_patch *to = so->p_patches;
	skin_patch_size count = so->p_patches_count;
	skin_sensor_type *sensor_types = so->p_sensor_types;
	skin_patch_id i;
	skin_module_id mi;

	for (i = 0; i < count; ++i)
	{
		to[i].id		= i;
		to[i].modules_begin	= sensor_types[from[i].layer].modules_begin + from[i].modules_begin;
		to[i].modules_end	= to[i].modules_begin + from[i].modules_count;

		/* set references from other structures to this one */
		for (mi = to[i].modules_begin; mi < to[i].modules_end; ++mi)
			so->p_modules[mi].patch_id = i;
	}
}

static void _copy_modules(skin_object *so, skink_module *from)
{
	skin_module *to = so->p_modules;
	skin_module_size count = so->p_modules_count;
	skin_sensor_type *sensor_types = so->p_sensor_types;
	skin_patch *patches = so->p_patches;
	skin_module_id i;
	skin_sensor_id si;

	for (i = 0; i < count; ++i)
	{
		to[i].id		= i;
		to[i].sensors_begin	= sensor_types[patches[to[i].patch_id].sensor_type_id].sensors_begin + from[i].sensors_begin;
		to[i].sensors_end	= to[i].sensors_begin + from[i].sensors_count;

		/* set references from other structures to this one */
		for (si = to[i].sensors_begin; si < to[i].sensors_end; ++si)
			so->p_sensors[si].module_id = i;
	}
}

static void _copy_sensors(skin_object *so, skink_sensor *from, int phase)
{
	skin_sensor *to = so->p_sensors;
	skin_sensor_size count = so->p_sensors_count;
	skin_sensor_id i;
	bool neighbor_memory_exists = so->p_sensor_neighbors != NULL;

	for (i = 0; i < count; ++i)
	{
		skin_sensor empty = {0};
		to[i] = empty;
		to[i].id			= i;
		to[i].sensor_type_id		= from[i].layer;
		to[i].sub_region_id		= from[i].sub_region;
		to[i].response			= 0;
		if (phase == SKIN_LOAD_FOR_CALIBRATION)
			continue;

		to[i].relative_position[0]	= from[i].position_nm[0] / 1000000000.0f;
		to[i].relative_position[1]	= from[i].position_nm[1] / 1000000000.0f;
		to[i].relative_position[2]	= from[i].position_nm[2] / 1000000000.0f;
		to[i].relative_orientation[0]	= from[i].orientation_nm[0] / 1000000000.0f;
		to[i].relative_orientation[1]	= from[i].orientation_nm[1] / 1000000000.0f;
		to[i].relative_orientation[2]	= from[i].orientation_nm[2] / 1000000000.0f;
		to[i].flattened_position[0]	= from[i].flattened_position_nm[0] / 1000000000.0f;
		to[i].flattened_position[1]	= from[i].flattened_position_nm[1] / 1000000000.0f;
		to[i].radius			= from[i].radius_nm / 1000000000.0f;
		to[i].robot_link		= from[i].robot_link;
		if (neighbor_memory_exists)
		{
			to[i].neighbors_count	= from[i].neighbors_count;
			to[i].neighbors		= so->p_sensor_neighbors + from[i].neighbors_begin;
		}
		else
		{
			to[i].neighbors_count	= 0;
			to[i].neighbors		= NULL;
		}
		/* TODO: While there is no one updating the global position, I'm doing this automatically, otherwise next 6 lines should be removed */
		to[i].global_position[0]	= to[i].relative_position[0];
		to[i].global_position[1]	= to[i].relative_position[1];
		to[i].global_position[2]	= to[i].relative_position[2];
		to[i].global_orientation[0]	= to[i].relative_orientation[0];
		to[i].global_orientation[1]	= to[i].relative_orientation[1];
		to[i].global_orientation[2]	= to[i].relative_orientation[2];
	}
}

static void _copy_neighbors(skin_object *so, skin_sensor_id *from)
{
	skin_sensor_id *to = so->p_sensor_neighbors;
	uint32_t count = so->p_sensor_neighbors_count;
	uint32_t i;

	for (i = 0; i < count; ++i)
		to[i] = from[i];
}

static void _update_patch_sensor_count(skin_object *so)
{
	skin_patch *patches = so->p_patches;
	skin_patch_size count = so->p_patches_count;
	skin_module *modules = so->p_modules;
	skin_patch_id i;
	skin_module_id mi;

	for (i = 0; i < count; ++i)
	{
		patches[i].p_sensors_count = 0;
		for (mi = patches[i].modules_begin; mi < patches[i].modules_end; ++mi)
			patches[i].p_sensors_count += modules[mi].sensors_end - modules[mi].sensors_begin;
	}
}

static void _copy_sub_regions(skin_object *so, skink_sub_region *from)
{
	skin_sub_region *to = so->p_sub_regions;
	skin_sub_region_size count = so->p_sub_regions_count;
	skin_sensor_type *sensor_types = so->p_sensor_types;
	skin_sensor_type_size sensor_types_count = so->p_sensor_types_count;
	skin_sensor_id *sensors_begins = so->p_sub_region_sensors_begins;
	skin_sensor_id *sensors_ends = so->p_sub_region_sensors_ends;
	skin_sub_region_id i;
	skin_sensor_type_id st;

	for (i = 0; i < count; ++i)
	{
		to[i].id			= i;
		for (st = 0; st < sensor_types_count; ++st)
		{
			/*
			 * sensors_begins and sensors_ends are packed arrays of [begin end) values for
			 * sensor locations of sub-regions.  Each block contains sensor_types_count values
			 * ([begin end) for each sensor type).  Sub-region i, sensor type j, has its
			 * begin and end in location i * sensor_types_count + j
			 *
			 * The actual sensors are placed in
			 *
			 *     sensor_types[j].begin (start of layer) + sub_region[i].layer[j].[begin end) (location in layer)
			 */
			sensors_begins[i * (uint32_t)sensor_types_count + st]
				= sensor_types[st].sensors_begin + from[i].layers[st].sensors_begin;
			sensors_ends[i * (uint32_t)sensor_types_count + st]
				= sensors_begins[i * (uint32_t)sensor_types_count + st] + from[i].layers[st].sensors_count;
		}
		/* sub-region's begins and ends arrays are pointers to so->p_sub_region_sensors_begins/ends */
		to[i].sensors_begins		= sensors_begins + i * (uint32_t)sensor_types_count;
		to[i].sensors_ends		= sensors_ends   + i * (uint32_t)sensor_types_count;

		to[i].region_indices_begin	= from[i].region_indices_begin;
		to[i].region_indices_end	= from[i].region_indices_begin + from[i].region_indices_count;
	}
}

static void _copy_regions(skin_object *so, skink_region *from)
{
	skin_region *to = so->p_regions;
	skin_region_size count = so->p_regions_count;
	skin_region_id i;

	for (i = 0; i < count; ++i)
	{
		to[i].id = i;
		to[i].sub_region_indices_begin	= from[i].sub_region_indices_begin;
		to[i].sub_region_indices_end	= from[i].sub_region_indices_begin + from[i].sub_region_indices_count;
	}
}

static void _copy_sub_region_indices(skin_object *so, skin_sub_region_id *from)
{
	skin_sub_region_id *to = so->p_sub_region_indices;
	skin_sub_region_index_size count = so->p_sub_region_indices_count;
	skin_sub_region_index_id i;

	for (i = 0; i < count; ++i)
		to[i] = from[i];
}

static void _copy_region_indices(skin_object *so, skin_region_id *from)
{
	skin_region_id *to = so->p_region_indices;
	skin_region_index_size count = so->p_region_indices_count;
	skin_region_index_id i;

	for (i = 0; i < count; ++i)
		to[i] = from[i];
}

static int _load_from_kernel(skin_object *so, int phase)
{
	skink_misc_datum	*misc_data;
	skink_sensor_layer	*kernel_sensor_layers		= NULL;
	skink_sensor		*kernel_sensors			= NULL;
	skink_module		*kernel_modules			= NULL;
	skink_patch		*kernel_patches			= NULL;
	skink_sub_region	*kernel_sub_regions		= NULL;
	skink_region		*kernel_regions			= NULL;
	skin_sub_region_id	*kernel_sub_region_indices	= NULL;
	skin_region_id		*kernel_region_indices		= NULL;
	skin_sensor_id		*kernel_sensor_neighbors	= NULL;
	int ret = SKIN_FAIL;

	/* get general information from the kernel */
	misc_data = _get_misc_and_check_phase(so, phase);
	if (misc_data == NULL)
		return SKIN_FAIL;

	/* make sure there are no compatiblity issues */
	if (!_compatible(so, misc_data))
	{
		skin_log_set_error(so, "Detected mismatch in object sizes between kernel and user spaces.  This is a serious error!\n"
				"--> Please report this error for a fix.  <--");
		goto exit_detach_misc;
	}

	/* attach to kernel memories */
	kernel_sensor_layers			= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	kernel_sensors				= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	kernel_modules				= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_MODULES);
	kernel_patches				= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_PATCHES);
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
	{
		kernel_sub_regions		= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGIONS);
		kernel_regions			= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_REGIONS);
		kernel_sub_region_indices	= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGION_INDICES);
		kernel_region_indices		= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_REGION_INDICES);
	}
	kernel_sensor_neighbors			= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
	if (!kernel_sensor_layers || !kernel_sensors || !kernel_modules || !kernel_patches
		|| (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL && (!kernel_sub_regions || !kernel_regions
				|| !kernel_sub_region_indices || !kernel_region_indices)))
	{
		skin_log_set_error(so, "Could not attach to kernel shared memories!");
		goto exit_detach_all;
	}

	/* allocate user memories */
	so->p_sensor_types_count			= SKINK_SENSOR_LAYERS_COUNT;
	so->p_sensors_count				= SKINK_SENSORS_COUNT;
	so->p_modules_count				= SKINK_MODULES_COUNT;
	so->p_patches_count				= SKINK_PATCHES_COUNT;
	if (phase != SKIN_LOAD_FOR_CALIBRATION)
	{
		so->p_sensor_neighbors_count		= SKINK_SENSOR_NEIGHBORS_COUNT;
		if (kernel_sensor_neighbors && so->p_sensor_neighbors_count > 0)
			so->p_sensor_neighbors		= malloc(so->p_sensor_neighbors_count * sizeof(*so->p_sensor_neighbors));
		else
			so->p_sensor_neighbors_count	= 0;
	}
	so->p_sensor_types				= malloc(so->p_sensor_types_count * sizeof(*so->p_sensor_types));
	so->p_sensors					= malloc(so->p_sensors_count * sizeof(*so->p_sensors));
	so->p_modules					= malloc(so->p_modules_count * sizeof(*so->p_modules));
	so->p_patches					= malloc(so->p_patches_count * sizeof(*so->p_patches));
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
	{
		so->p_sub_regions_count			= SKINK_SUB_REGIONS_COUNT;
		so->p_regions_count			= SKINK_REGIONS_COUNT;
		so->p_sub_region_indices_count		= SKINK_SUB_REGION_INDICES_COUNT;
		so->p_region_indices_count		= SKINK_REGION_INDICES_COUNT;
		so->p_sub_region_sensors_begins_count	= so->p_sensor_types_count * (uint32_t)so->p_sub_regions_count;
		so->p_sub_region_sensors_ends_count	= so->p_sensor_types_count * (uint32_t)so->p_sub_regions_count;
		so->p_sub_regions			= malloc(so->p_sub_regions_count * sizeof(*so->p_sub_regions));
		so->p_regions				= malloc(so->p_regions_count * sizeof(*so->p_regions));
		so->p_sub_region_indices		= malloc(so->p_sub_region_indices_count * sizeof(*so->p_sub_region_indices));
		so->p_region_indices			= malloc(so->p_region_indices_count * sizeof(*so->p_region_indices));
		so->p_sub_region_sensors_begins		= malloc(so->p_sub_region_sensors_begins_count * sizeof(*so->p_sub_region_sensors_begins));
		so->p_sub_region_sensors_ends		= malloc(so->p_sub_region_sensors_ends_count * sizeof(*so->p_sub_region_sensors_ends));
	}
	if (!so->p_sensor_types || !so->p_sensors || !so->p_modules || !so->p_patches || (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL
		&& (!so->p_sub_regions || !so->p_regions || !so->p_sub_region_indices || !so->p_region_indices
			|| !so->p_sub_region_sensors_begins || !so->p_sub_region_sensors_ends)))
	{
		skin_log_set_error(so, "Could not acquire memory for main data structures");
		goto exit_no_mem;
	}

	/* get the structures over here */
	_copy_sensor_types(so, kernel_sensor_layers);
	_copy_patches(so, kernel_patches);
	_copy_modules(so, kernel_modules);
	_copy_sensors(so, kernel_sensors, phase);
	_copy_neighbors(so, kernel_sensor_neighbors);
	_update_patch_sensor_count(so);
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
	{
		_copy_sub_regions(so, kernel_sub_regions);
		_copy_regions(so, kernel_regions);
		_copy_sub_region_indices(so, kernel_sub_region_indices);
		_copy_region_indices(so, kernel_region_indices);
	}

	skin_log("Data obtained from module:");
	skin_log("\tSensor types count: %u", so->p_sensor_types_count);
	skin_log("\tSensors count: %u", so->p_sensors_count);
	skin_log("\tModules count: %u", so->p_modules_count);
	skin_log("\tPatches count: %u", so->p_patches_count);
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
	{
		skin_log("\tRegions count: %u", so->p_regions_count);
		skin_log("\tSubRegions count: %u", so->p_sub_regions_count);
		skin_log("\tRegion indices count: %llu", (unsigned long long)so->p_region_indices_count);
		skin_log("\tSubRegion indices count: %llu", (unsigned long long)so->p_sub_region_indices_count);
	}
	ret = EXIT_SUCCESS;
exit_detach_all:
	if (kernel_sensor_layers)	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	if (kernel_sensors)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	if (kernel_modules)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MODULES);
	if (kernel_patches)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_PATCHES);
	if (kernel_sub_regions)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGIONS);
	if (kernel_regions)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_REGIONS);
	if (kernel_sub_region_indices)	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGION_INDICES);
	if (kernel_region_indices)	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_REGION_INDICES);
	if (kernel_sensor_neighbors)	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
exit_detach_misc:
	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	return ret;
exit_no_mem:
	ret = SKIN_NO_MEM;
	CLEAN_FREE(so->p_sensor_types);
	CLEAN_FREE(so->p_sensors);
	CLEAN_FREE(so->p_modules);
	CLEAN_FREE(so->p_patches);
	CLEAN_FREE(so->p_sub_regions);
	CLEAN_FREE(so->p_regions);
	CLEAN_FREE(so->p_sub_region_indices);
	CLEAN_FREE(so->p_region_indices);
	CLEAN_FREE(so->p_sub_region_sensors_begins);
	CLEAN_FREE(so->p_sub_region_sensors_ends);
	CLEAN_FREE(so->p_sensor_neighbors);
	goto exit_detach_all;
}

static void _calibrate_sensors(skin_object *so, skink_sensor *to)
{
	skin_sensor *from = so->p_sensors;
	skin_sensor_size count = so->p_sensors_count;
	skin_sensor_id i;

	for (i = 0; i < count; ++i)
	{
		to[i].position_nm[0]		= from[i].relative_position[0] * 1000000000.0f;
		to[i].position_nm[1]		= from[i].relative_position[1] * 1000000000.0f;
		to[i].position_nm[2]		= from[i].relative_position[2] * 1000000000.0f;
		to[i].orientation_nm[0]		= from[i].relative_orientation[0] * 1000000000.0f;
		to[i].orientation_nm[1]		= from[i].relative_orientation[1] * 1000000000.0f;
		to[i].orientation_nm[2]		= from[i].relative_orientation[2] * 1000000000.0f;
		to[i].flattened_position_nm[0]	= from[i].flattened_position[0] * 1000000000.0f;
		to[i].flattened_position_nm[1]	= from[i].flattened_position[1] * 1000000000.0f;
		to[i].radius_nm			= from[i].radius * 1000000000.0f;
		to[i].robot_link		= from[i].robot_link;
	}
}

static void _calibrate_neighbors(skin_object *so, skin_sensor_id *to, skink_sensor *ksensors)
{
	skin_sensor_id *from = so->p_sensor_neighbors;
	uint32_t count = so->p_sensor_neighbors_count;
	skin_sensor *sensors = so->p_sensors;
	skin_sensor_size sensors_count = so->p_sensors_count;
	uint32_t i;
	uint32_t cur;
	skin_sensor_id si;

	for (i = 0; i < count; ++i)
		to[i] = from[i];

	/* let sensors know their neighbors */
	cur = 0;
	for (si = 0; si < sensors_count; ++si)
	{
		ksensors[si].neighbors_begin	= cur;
		ksensors[si].neighbors_count	= sensors[si].neighbors_count;
		cur += sensors[si].neighbors_count;
	}
}

static int _calibrate_kernel(skin_object *so)
{
	skink_misc_datum	*misc_data;
	skink_sensor		*kernel_sensors = NULL;
	skin_sensor_id		*kernel_neighbors = NULL;
	FILE *cmd;
	bool is_contiguous;
	int ret = SKIN_FAIL;

	misc_data = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	if (misc_data == NULL)
	{
		skin_log_set_error(so, "Could not attach to misc shared memory!");
		return SKIN_FAIL;
	}
	if (SKINK_CALIBRATION_TOO_EARLY)
	{
		skin_log_set_error(so, "The skin kernel is not in calibration phase yet.  Waiting...");
		while (SKINK_CALIBRATION_TOO_EARLY && !SKINK_CALIBRATION_POSSIBLE)
			usleep(1000);
	}
	if (!SKINK_CALIBRATION_POSSIBLE)
	{
		skin_log_set_error(so, "The skin kernel has passed calibration phase, which means either two instances of the calibration program are running\n"
				"or someone is messing with the kernel's /proc file");
		goto exit_detach_misc;
	}
	if (SKINK_SENSORS_COUNT != so->p_sensors_count)
	{
		/* could happen if wrong calibration file is loaded */
		skin_log_set_error(so, "Mismatch between number of sensors in the skin kernel and number of sensors calibrated.\nDid you load the wrong file?");
		ret = SKIN_FILE_INVALID;
		goto exit_detach_misc;
	}

	kernel_sensors = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	if (!kernel_sensors)
	{
		skin_log_set_error(so, "Could not attach to kernel shared memories!");
		goto exit_detach_misc;
	}
	_calibrate_sensors(so, kernel_sensors);

	SKINK_SENSOR_NEIGHBORS_COUNT = so->p_sensor_neighbors_count;

	/* TODO: Test not creating the array and see if Skinware can handle it */
	kernel_neighbors = skin_rt_shared_memory_alloc(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS,
						SKINK_SENSOR_NEIGHBORS_COUNT * sizeof(*kernel_neighbors), &is_contiguous);
	if (!kernel_neighbors)
	{
		skin_log_set_error(so, "Could not acquire memory for sensor neighbors.  These data will not be available to others!");
		SKINK_SENSOR_NEIGHBORS_COUNT = 0;
	}
	else if (!is_contiguous)
		skin_log_set_error(so, "Warning - Using discontiguous memory!");
	if (kernel_neighbors)
		_calibrate_neighbors(so, kernel_neighbors, kernel_sensors);
	else
	{
		skin_sensor_id i;
		for (i = 0; i < so->p_sensors_count; ++i)
		{
			kernel_sensors[i].neighbors_begin	= 0;
			kernel_sensors[i].neighbors_count	= 0;
		}
	}
	cmd = fopen("/proc/"SKINK_COMMAND_PROCFS_ENTRY, "w");
	if (cmd == NULL)
	{
		skin_log_set_error(so, "Could not open skin kernel's command /proc file for writing");
		goto exit_detach_all;
	}
	fprintf(cmd, "clbr");
	fclose(cmd);

	/*
	 * wait for the kernel to change state.  This is to make sure the kernel attaches to
	 * the sensor neighbors memory, therefore detaching here wouldn't delete it.
	 */
	while (SKINK_CALIBRATION_POSSIBLE)
		usleep(1000);

	/* make sure the skin is not used afterwards */
	skin_object_free(so);
	ret = SKIN_SUCCESS;
exit_detach_all:
	if (kernel_neighbors)
		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
exit_detach_misc:
	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	return ret;
}

/* check whether regionalization data and module data reported by kernel are compatible */
static bool _check_regionalization_data(skin_object *so, skink_sensor_layer *klayers, skink_module *kmodules,
		skin_sensor_id *sensor_map, skin_module_size modules_count)
{
	skin_sensor_type *sensor_types = so->p_sensor_types;
	skin_module_id i;

	for (i = 0; i < modules_count; ++i)
	{
		skin_sensor_id   layer_sensors_begin = sensor_types[kmodules[i].layer].sensors_begin;
		skin_sensor_size layer_sensors_count = klayers[kmodules[i].layer].sensors_count;
		if ((uint64_t)sensor_map[layer_sensors_begin + kmodules[i].sensors_begin] - layer_sensors_begin
				+ kmodules[i].sensors_count > layer_sensors_count)
			return false;
	}
	return true;
}

struct sub_region_region_indices
{
	skin_region_id *ids;
	size_t size;
	size_t mem_size;
};

static int _enlarge_region_indices(struct sub_region_region_indices *ri)
{
	skin_region_id *enlarged;
	size_t new_size;

	if (ri->size < ri->mem_size)
		return 0;

	new_size = ri->mem_size * 2;
	if (new_size < 32)
		new_size = 32;
	enlarged = realloc(ri->ids, new_size * sizeof(*enlarged));
	if (enlarged == NULL)
		return -1;
	ri->ids = enlarged;
	ri->mem_size = new_size;

	return 0;
}

static void _free_region_indices(skin_object *so, struct sub_region_region_indices *region_indices)
{
	skin_sub_region_size sub_regions_count = so->p_sub_regions_count;
	skin_sub_region_id i;

	if (region_indices)
		for (i = 0; i < sub_regions_count; ++i)
			free(region_indices[i].ids);
	free(region_indices);
}

/* given sub-region indices in regions, calculates region indices for sub-regions */
struct sub_region_region_indices *_calculate_region_indices(skin_object *so)
{
	struct sub_region_region_indices *region_indices = NULL;
	skin_region *regions = so->p_regions;
	skin_sub_region_id *sub_region_indices = so->p_sub_region_indices;
	skin_region_size regions_count = so->p_regions_count;
	skin_sub_region_size sub_regions_count = so->p_sub_regions_count;
	skin_region_id i;
	skin_sub_region_id sri;
	skin_sub_region_index_id srii;

	region_indices = malloc(sub_regions_count * sizeof(*region_indices));
	if (region_indices == NULL)
		goto exit_no_mem;
	for (sri = 0; sri < sub_regions_count; ++sri)
		region_indices[sri] = (struct sub_region_region_indices){0};

	for (i = 0; i < regions_count; ++i)
	{
		skin_sub_region_index_id begin = regions[i].sub_region_indices_begin;
		skin_sub_region_index_id end = regions[i].sub_region_indices_end;
		for (srii = begin; srii < end; ++srii)
		{
			struct sub_region_region_indices *cur = &region_indices[sub_region_indices[srii]];

			_enlarge_region_indices(cur);
			cur->ids[cur->size++] = i;
		}
	}

	return region_indices;
exit_no_mem:
	_free_region_indices(so, region_indices);
	return NULL;
}

static void _regionalize_regions(skin_object *so, skink_region *to)
{
	skin_region *from = so->p_regions;
	skin_region_size count = so->p_regions_count;
	skin_region_id i;

	for (i = 0; i < count; ++i)
	{
		to[i].id			= i;
		to[i].sub_region_indices_begin	= from[i].sub_region_indices_begin;
		to[i].sub_region_indices_count	= from[i].sub_region_indices_end - from[i].sub_region_indices_begin;
	}
}

static void _regionalize_sub_regions(skin_object *so, skink_sub_region *to, struct sub_region_region_indices *region_indices)
{
	skin_sensor_id *sensors_begins = so->p_sub_region_sensors_begins;
	skin_sensor_id *sensors_ends = so->p_sub_region_sensors_ends;
	skin_sub_region_size count = so->p_sub_regions_count;
	skin_sensor_type_size sensor_types_count = so->p_sensor_types_count;
	skin_region_id cur_region_index = 0;
	skin_sub_region_id i;
	skin_sensor_type_id sti;

	for (i = 0; i < count; ++i)
	{
		to[i].id				= i;
		to[i].region_indices_begin		= cur_region_index;
		to[i].region_indices_count		= region_indices[i].size;
		for (sti = 0; sti < sensor_types_count; ++sti)
		{
			to[i].layers[sti].sensors_begin	= sensors_begins[i * (uint32_t)sensor_types_count + sti];
			to[i].layers[sti].sensors_count	= sensors_ends[i * (uint32_t)sensor_types_count + sti]
							- sensors_begins[i * (uint32_t)sensor_types_count + sti];
		}
		cur_region_index += region_indices[i].size;
	}
}

static void _regionalize_regions_indices(skin_object *so, skin_region_id *to, struct sub_region_region_indices *region_indices)
{
	skin_sub_region_size count = so->p_sub_regions_count;
	skin_region_id cur_region_index = 0;
	skin_sub_region_id i;
	skin_region_index_id rii;

	for (i = 0; i < count; ++i)
	{
		for (rii = 0; rii < region_indices[i].size; ++rii)
			to[cur_region_index + rii] = region_indices[i].ids[rii];
		cur_region_index += region_indices[i].size;
	}
}

static void _regionalize_sub_regions_indices(skin_object *so, skin_sub_region_id *to)
{
	skin_sub_region_id *from = so->p_sub_region_indices;
	skin_sub_region_index_size count = so->p_sub_region_indices_count;
	skin_sub_region_index_id i;

	for (i = 0; i < count; ++i)
		to[i] = from[i];
}

static int _regionalize_kernel(skin_object *so)
{
	skink_misc_datum		*misc_data;
	skink_sub_region		*kernel_sub_regions = NULL;
	skink_region			*kernel_regions = NULL;
	skin_sub_region_id		*kernel_sub_region_indices = NULL;
	skin_region_id			*kernel_region_indices = NULL;
	skink_sensor_layer		*kernel_sensor_layers = NULL;
	skink_sensor			*kernel_sensors = NULL;
	skink_module			*kernel_modules = NULL;
	skin_sensor_id			*kernel_sensor_neighbors = NULL;
	skin_sensor_id			*sensor_map = NULL;
	skink_sensor			*temp_kernel_sensors = NULL;
	struct sub_region_region_indices *region_indices = NULL;
	skin_sensor_type		*sensor_types = so->p_sensor_types;
	skin_sensor			*sensors = so->p_sensors;
	skin_sensor_type_size		sensor_types_count = so->p_sensor_types_count;
	skin_sensor_size		sensors_count = so->p_sensors_count;
	FILE *cmd;
	skin_module_id layer_modules_begin;
	skin_sensor_type_id sti;
	skin_sensor_id si;
	skin_module_id mi;
	skin_sub_region_id sri;
	uint32_t ni;
	int ret = SKIN_FAIL;

	misc_data = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	if (misc_data == NULL)
	{
		skin_log_set_error(so, "Could not attach to misc shared memory!");
		return SKIN_FAIL;
	}
	if (SKINK_REGIONALIZATION_TOO_EARLY)
	{
		skin_log_set_error(so, "The skin kernel is not in regionalization phase yet.  Waiting...");
		while (SKINK_REGIONALIZATION_TOO_EARLY && !SKINK_REGIONALIZATION_POSSIBLE)
			usleep(1000);
	}
	if (!SKINK_REGIONALIZATION_POSSIBLE)
	{
		skin_log_set_error(so, "The skin kernel has passed regionalization phase, which means either two instances of the regionalization program are running\n"
				"or someone is messing with the kernel's /proc file");
		goto exit_detach_misc;
	}
	if (SKINK_SENSORS_COUNT != sensors_count || SKINK_SENSOR_LAYERS_COUNT != sensor_types_count)
	{
		/* could happen if wrong regionalization file is loaded */
		skin_log_set_error(so, "Mismatch between number of sensors, sensor types or modules in the skin kernel and number of sensors in the regions.\n"
				"Did you load the wrong file?");
		ret = SKIN_FILE_INVALID;
		goto exit_detach_misc;
	}
#define SHARED_MEM_ALLOC(name_c, name_s)\
	do {\
		bool _is_contiguous;\
		kernel_##name_s = skin_rt_shared_memory_alloc(SKINK_RT_SHARED_MEMORY_NAME_##name_c,\
				so->p_##name_s##_count * sizeof(*kernel_##name_s),\
				&_is_contiguous);\
		if (kernel_##name_s == NULL)\
		{\
			skin_log_set_error(so, "Could not acquire memory for "#name_s"!");\
			goto exit_no_mem;\
		}\
		else if (!_is_contiguous)\
			skin_log_set_error(so, "Warning - Using discontiguous memory!");\
	} while (0)

	/* TODO: fail something at this step and see if Skinware is recoverable */
	sensor_map		= malloc(sensors_count * sizeof(*sensor_map));
	temp_kernel_sensors	= malloc(sensors_count * sizeof(*temp_kernel_sensors));
	if (!sensor_map || !temp_kernel_sensors)
	{
		skin_log_set_error(so, "Could not acquire memory for sensor map");
		goto exit_no_mem;
	}
	kernel_sensor_layers	= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	kernel_sensors		= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	kernel_modules		= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_MODULES);
	kernel_sensor_neighbors	= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
	if (!kernel_sensors || !kernel_modules || !kernel_sensor_layers)
	{
		skin_log_set_error(so, "Could not attach to kernel shared memories!");
		goto exit_detach_all;
	}

	/*
	 * create a copy and a map for later use
	 *
	 * At first, the sensor map is a global id of sensor, which during regionalization_end() has
	 * been properly rearranged.  Note therefore that x and sensors[x].id may be different
	 */
	for (si = 0; si < sensors_count; ++si)
		temp_kernel_sensors[si] = kernel_sensors[si];
	for (sti = 0; sti < sensor_types_count; ++sti)
	{
		skin_sensor_size begin = sensor_types[sti].sensors_begin;
		skin_sensor_size end = sensor_types[sti].sensors_end;
		for (si = begin; si < end; ++si)
			sensor_map[si] = sensor_types[sti].sensors_begin + sensors[si].id;
	}

	/* before going forward, do some extra checking */
	if (!_check_regionalization_data(so, kernel_sensor_layers, kernel_modules, sensor_map, SKINK_MODULES_COUNT))
	{
		skin_log_set_error(so, "Mismatch between module sizes in the skin kernel and those implied by the regions.\n"
				"Has the skin been reorganized and the file is old?");
		goto exit_detach_all;
	}

	/* ask the kernel to pause writers, because sensors may be rearranged */
	SKINK_REGIONALIZATION_SYNC = 1;
	while (SKINK_REGIONALIZATION_SYNC == 1)
		usleep(1000);

	/* allocate region/sub-region memories and attach to sensors and modules memories of kernel */
	SHARED_MEM_ALLOC(SUB_REGIONS, sub_regions);
	SHARED_MEM_ALLOC(REGIONS, regions);
	SHARED_MEM_ALLOC(SUB_REGION_INDICES, sub_region_indices);
	SHARED_MEM_ALLOC(REGION_INDICES, region_indices);
	SKINK_SUB_REGIONS_COUNT		= so->p_sub_regions_count;
	SKINK_REGIONS_COUNT		= so->p_regions_count;
	SKINK_SUB_REGION_INDICES_COUNT	= so->p_sub_region_indices_count;
	SKINK_REGION_INDICES_COUNT	= so->p_region_indices_count;

	/* proceed with regionalization */
	region_indices = _calculate_region_indices(so);
	if (region_indices == NULL)
	{
		skin_log_set_error(so, "Could not acquire memory for region indices vector");
		goto exit_no_mem;
	}
	_regionalize_regions(so, kernel_regions);
	_regionalize_sub_regions(so, kernel_sub_regions, region_indices);
	_regionalize_regions_indices(so, kernel_region_indices, region_indices);
	_regionalize_sub_regions_indices(so, kernel_sub_region_indices);

	/* rearrange the sensors */
	for (sti = 0; sti < so->p_sensor_types_count; ++sti)
	{
		skin_sensor_id begin = so->p_sensor_types[sti].sensors_begin;
		skin_sensor_id end = so->p_sensor_types[sti].sensors_end;
		for (si = begin; si < end; ++si)
			kernel_sensors[sensor_map[si]] = temp_kernel_sensors[si];
	}
	/* update the modules */
	layer_modules_begin = 0;
	for (sti = 0; sti < so->p_sensor_types_count; ++sti)
	{
		skin_module_id begin = layer_modules_begin;
		skin_module_id end = layer_modules_begin + kernel_sensor_layers[sti].modules_count;
		for (mi = begin; mi < end; ++mi)
			kernel_modules[mi].sensors_begin += so->p_sensor_types[sti].sensors_begin;
		layer_modules_begin += kernel_sensor_layers[sti].modules_count;
	}
	for (mi = 0; mi < SKINK_MODULES_COUNT; ++mi)
		kernel_modules[mi].sensors_begin = sensor_map[kernel_modules[mi].sensors_begin];	/* the kernel would make this relative to layer */

	/* let each sensor know which sub-region it belongs too */
	for (sri = 0; sri < so->p_sub_regions_count; ++sri)
		for (sti = 0; sti < so->p_sensor_types_count; ++sti)
		{
			skin_sensor_id begin = so->p_sensor_types[sti].sensors_begin
							+ kernel_sub_regions[sri].layers[sti].sensors_begin;
			skin_sensor_id end = begin + kernel_sub_regions[sri].layers[sti].sensors_count;
			for (si = begin; si < end; ++si)
				kernel_sensors[si].sub_region = sri;
		}

	/*
	 * update the sensor ids for the kernel to understand the sensor map.
	 * Note that, the kernel would revert the ids afterwards
	 */
	for (si = 0; si < so->p_sensors_count; ++si)
		kernel_sensors[si].id = so->p_sensors[si].id;
	if (kernel_sensor_neighbors)
		for (ni = 0; ni < SKINK_SENSOR_NEIGHBORS_COUNT; ++ni)
			kernel_sensor_neighbors[ni] = sensor_map[kernel_sensor_neighbors[ni]];

	/* let the kernel know that we're done */
	SKINK_REGIONALIZATION_SYNC = 1;
	cmd = fopen("/proc/"SKINK_COMMAND_PROCFS_ENTRY, "w");
	if (cmd == NULL)
	{
		skin_log_set_error(so, "Could not open skin kernel's command /proc file for writing");
		goto exit_detach_all;
	}
	fprintf(cmd, "rgn");
	fclose(cmd);

	/*
	 * wait for the kernel to change state.  This is to make sure the kernel attaches to
	 * the region/sub-region related memory, therefore detaching here wouldn't delete it.
	 */
	while (SKINK_REGIONALIZATION_POSSIBLE)
		usleep(1000);

	/* make sure the skin is not used afterwards */
	skin_object_free(so);
	ret = SKIN_SUCCESS;
exit_detach_all:
	if (kernel_sub_regions)		skin_rt_shared_memory_free(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGIONS);
	if (kernel_regions)		skin_rt_shared_memory_free(SKINK_RT_SHARED_MEMORY_NAME_REGIONS);
	if (kernel_sub_region_indices)	skin_rt_shared_memory_free(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGION_INDICES);
	if (kernel_region_indices)	skin_rt_shared_memory_free(SKINK_RT_SHARED_MEMORY_NAME_REGION_INDICES);
	if (kernel_sensor_layers)	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	if (kernel_sensors)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	if (kernel_modules)		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MODULES);
	if (kernel_sensor_neighbors)	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
	free(sensor_map);
	free(temp_kernel_sensors);
	_free_region_indices(so, region_indices);
exit_detach_misc:
	skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	return ret;
exit_no_mem:
	ret = SKIN_NO_MEM;
	goto exit_detach_all;
}

#ifndef NDEBUG
static void _dump_structures(skin_object *so)
{
	skin_sensor_type_id sti;
	skin_sensor_id si;
	skin_module_id mi;
	skin_patch_id pi;
	skin_region_id ri;
	skin_sub_region_id sri;
	skin_region_index_id rii;
	skin_sub_region_index_id srii;

	skin_log("Debug: Data structure built:");
	skin_log("%u sensor types:", so->p_sensor_types_count);
	for (sti = 0; sti < so->p_sensor_types_count; ++sti)
		skin_log("\tId: %u, name: %s, sensors: [%u, %u), modules: [%u, %u), patches: [%u, %u)",
			so->p_sensor_types[sti].id, so->p_sensor_types[sti].name?so->p_sensor_types[sti].name:"<none>",
			so->p_sensor_types[sti].sensors_begin, so->p_sensor_types[sti].sensors_end,
			so->p_sensor_types[sti].modules_begin, so->p_sensor_types[sti].modules_end,
			so->p_sensor_types[sti].patches_begin, so->p_sensor_types[sti].patches_end);
	skin_log("%u sensors:", so->p_sensors_count);
	for (si = 0; si < so->p_sensors_count; ++si)
	{
		skin_sensor_id sj;

		skin_log_no_newline("\tId: %u, sub_region: %u, module: %u, type: %u"
				", position: (%f, %f, %f), on robot link: %u"
				", neighbors: {",
				so->p_sensors[si].id, so->p_sensors[si].sub_region_id, so->p_sensors[si].module_id,
				so->p_sensors[si].sensor_type_id, so->p_sensors[si].relative_position[0],
				so->p_sensors[si].relative_position[1], so->p_sensors[si].relative_position[2],
				so->p_sensors[si].robot_link);
			for (sj = 0; sj < so->p_sensors[si].neighbors_count; ++sj)
				skin_log_no_newline("%s%u", (sj == 0?"":", "), so->p_sensors[si].neighbors[sj]);
		skin_log("}");
	}
	skin_log("%u modules:", so->p_modules_count);
	for (mi = 0; mi < so->p_modules_count; ++mi)
		skin_log("\tId: %u, patch: %u, type: %u, sensors: [%u, %u)",
			so->p_modules[mi].id, so->p_modules[mi].patch_id, so->p_modules[mi].sensor_type_id,
			so->p_modules[mi].sensors_begin, so->p_modules[mi].sensors_end);
	skin_log("%u patches:", so->p_patches_count);
	for (pi = 0; pi < so->p_patches_count; ++pi)
		skin_log("\tId: %u, type: %u, modules: [%u, %u)",
			so->p_patches[pi].id, so->p_patches[pi].sensor_type_id,
			so->p_patches[pi].modules_begin, so->p_patches[pi].modules_end);
	if (so->p_sub_regions && so->p_regions)
	{
		skin_log("%u sub_regions:", so->p_sub_regions_count);
		for (sri = 0; sri < so->p_sub_regions_count; ++sri)
		{
			skin_log_no_newline("\tId: %u, sensors:", so->p_sub_regions[sri].id);
			for (sti = 0; sti < so->p_sensor_types_count; ++sti)
				skin_log_no_newline(" [%u, %u)", so->p_sub_regions[sri].sensors_begins[sti],
						so->p_sub_regions[sri].sensors_ends[sti]);
			skin_log_no_newline(", regions: {");
			for (rii = so->p_sub_regions[sri].region_indices_begin; rii < so->p_sub_regions[sri].region_indices_end; ++rii)
				skin_log_no_newline("%s%llu", (rii == so->p_sub_regions[sri].region_indices_begin?"":", "),
						(unsigned long long)so->p_region_indices[rii]);
			skin_log("}");
		}
		skin_log("%u regions:", so->p_regions_count);
		for (ri = 0; ri < so->p_regions_count; ++ri)
		{
			skin_log_no_newline("\tId: %u, sub_regions: {", so->p_regions[ri].id);
			for (srii = so->p_regions[ri].sub_region_indices_begin; srii < so->p_regions[ri].sub_region_indices_end; ++srii)
				skin_log_no_newline("%s%llu", (srii == so->p_regions[ri].sub_region_indices_begin?"":", "),
						(unsigned long long)so->p_sub_region_indices[srii]);
			skin_log("}");
		}
	}
}
#endif

static void _set_back_references(skin_object *so)
{
	skin_sensor_type_id sti;
	skin_sensor_id si;
	skin_module_id mi;
	skin_patch_id pi;
	skin_region_id ri;
	skin_sub_region_id sri;

	for (sti = 0; sti < so->p_sensor_types_count; ++sti)
		so->p_sensor_types[sti].p_object = so;
	for (si = 0; si < so->p_sensors_count; ++si)
		so->p_sensors[si].p_object = so;
	for (mi = 0; mi < so->p_modules_count; ++mi)
		so->p_modules[mi].p_object = so;
	for (pi = 0; pi < so->p_patches_count; ++pi)
		so->p_patches[pi].p_object = so;
	if (so->p_regions && so->p_sub_regions)
	{
		for (ri = 0; ri < so->p_regions_count; ++ri)
			so->p_regions[ri].p_object = so;
		for (sri = 0; sri < so->p_sub_regions_count; ++sri)
			so->p_sub_regions[sri].p_object = so;
	}
	so->p_reader.p_object = so;
	so->p_service_manager.p_object = so;
}

static int _load_common(skin_object *so, int phase)
{
	int res;

	/* free the skin in case of calling this function a second time */
	skin_object_free(so);

	skin_log_init();
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
		skin_log("Loading skin...");
	else if (phase == SKIN_LOAD_FOR_CALIBRATION)
		skin_log("Preparing skin for calibration...");
	else if (phase == SKIN_LOAD_FOR_REGIONALIZATION)
		skin_log("Preparing skin for regionalization...");
	else
	{
		skin_log("Internal error: bad call to _load_common");
		return SKIN_FAIL;
	}
	skin_log("Note: skin kernel is version: %s", SKINK_VERSION);
	skin_log("Note: skin library is version: %s", SKIN_VERSION);
	skin_log_no_newline("- Loading data from skin kernel...  ");

	res = _load_from_kernel(so, phase);
	if (res < 0)
		goto exit_fail;
	skin_log("done");

	/* add a reference to the object to whoever needs it */
	_set_back_references(so);

	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
	{
		skin_log_no_newline("- Preparing service manager...  ");
		res = skin_service_manager_internal_initialize(&so->p_service_manager);
		if (res < 0)
			goto exit_fail;
	}

	skin_log_no_newline("- Preparing readers...  ");
	res = skin_reader_internal_initialize(&so->p_reader, so->p_sensor_types_count, phase != SKIN_LOAD_FROM_INITIALIZED_KERNEL);
	if (res < 0)
		goto exit_uninit_services;
	skin_log("done");

	if (phase != SKIN_LOAD_FROM_INITIALIZED_KERNEL)
	{
		skin_log("- Starting acquisition...  ");
		res = skin_reader_start(&so->p_reader, SKIN_ALL_SENSOR_TYPES, SKIN_ACQUISITION_ASAP, 0);
		if (res < 0)
			goto exit_uninit_readers;
		skin_log("  Starting acquisition... done");
	}

	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
		skin_log("Loading skin... done");
	else if (phase == SKIN_LOAD_FOR_CALIBRATION)
		skin_log("Preparing skin for calibration... done");
	else if (phase == SKIN_LOAD_FOR_REGIONALIZATION)
		skin_log("Preparing skin for regionalization... done");

#if SKIN_DEBUG
	_dump_structures(so);
#endif

	return SKIN_SUCCESS;
exit_uninit_readers:
	skin_reader_internal_terminate(&so->p_reader);
exit_uninit_services:
	if (phase == SKIN_LOAD_FROM_INITIALIZED_KERNEL)
		skin_service_manager_internal_terminate(&so->p_service_manager);
exit_fail:
	return res;
}

int skin_object_load(skin_object *so)
{
	return _load_common(so, SKIN_LOAD_FROM_INITIALIZED_KERNEL);
}

int skin_object_calibration_begin(skin_object *so)
{
	return _load_common(so, SKIN_LOAD_FOR_CALIBRATION);
}

int skin_object_calibration_end(skin_object *so, const char *cache)
{
	uint32_t ni;
	uint32_t cur;
	skin_sensor_id si;

	skin_reader_internal_terminate(&so->p_reader);

	/*
	 * TODO: Do the flattening and get information such as flattened_position, robot links and sensor neighbors
	 * and possibly sensor radius
	 */
	/* for now, create a test map of each sensor neighboring only itself */
	so->p_sensor_neighbors_count	= so->p_sensors_count;
	so->p_sensor_neighbors		= malloc(so->p_sensor_neighbors_count * sizeof(*so->p_sensor_neighbors));
	if (!so->p_sensor_neighbors)
	{
		skin_log_set_error(so, "Could not acquire memory for main data structures");
		so->p_sensor_neighbors = 0;
		return SKIN_NO_MEM;
	}
	for (ni = 0; ni < so->p_sensor_neighbors_count; ++ni)
		so->p_sensor_neighbors[ni] = ni;
	cur = 0;
	for (si = 0; si < so->p_sensors_count; ++si)
	{
		so->p_sensors[si].neighbors		= so->p_sensor_neighbors + cur;
		so->p_sensors[si].neighbors_count	= 1;
		cur += so->p_sensors[si].neighbors_count;
	}
	/* Note: end of flattenning test */

	/* cache the results */
	if (cache)
	{
		FILE *cch;

		cch = fopen(cache, "w");
		if (cch == NULL)
		{
			skin_log_set_error(so, "Could not open cache file \"%s\" for writing.  Results will not be cached.", cache);
			goto exit_calibrate;
		}

		fprintf(cch, "%u %u\n", so->p_sensors_count, so->p_sensor_neighbors_count);
		for (si = 0; si < so->p_sensors_count; ++si)
		{
			skin_sensor_id sj;

			fprintf(cch, "%f %f %f   %f %f %f   %f %f  %f  %u  %u",
					so->p_sensors[si].relative_position[0],
					so->p_sensors[si].relative_position[1],
					so->p_sensors[si].relative_position[2],
					so->p_sensors[si].relative_orientation[0],
					so->p_sensors[si].relative_orientation[1],
					so->p_sensors[si].relative_orientation[2],
					so->p_sensors[si].flattened_position[0],
					so->p_sensors[si].flattened_position[1],
					so->p_sensors[si].radius,
					so->p_sensors[si].robot_link,
					so->p_sensors[si].neighbors_count);
			for (sj = 0; sj < so->p_sensors[si].neighbors_count; ++sj)
				fprintf(cch, " %u", so->p_sensors[si].neighbors[sj]);
			fprintf(cch, "\n");
		}

		/* print the format if user wants to modify the cache manually */
		fprintf(cch, "\nFormat:\n"
				"sensors-count  total-neighbors-count\n"
				"(sensors-count lines) position (3 values)  orientation(3 values)  flattened-position (2 values)  "
				"radius  robot-link  neighbors-count neighbors (neighbors-count numbers)\n");
		fclose(cch);
	}

exit_calibrate:
	return _calibrate_kernel(so);
}

int skin_object_calibration_reload(skin_object *so, const char *cache)
{
	FILE *cch;
	unsigned long long sensors_count, neighbors_count;
	skin_sensor_id si, sj;
	uint32_t cur;
	int ret;

	if (!cache)
		return SKIN_FAIL;

	cch = fopen(cache, "r");
	/*
	 * could be that this is called just to check if the cache exists.
	 * Therefore if it doesn't exist, don't log error
	 */
	if (cch == NULL)
		return SKIN_NO_FILE;

	skin_log("Calibrating skin from cache...");
	fscanf(cch, "%llu %llu", &sensors_count, &neighbors_count);
	SKIN_CHECK_FILE_INTEGRITY(so, cch, "calibration cache");
	if (sensors_count == 0 || neighbors_count == 0)
	{
		skin_log_set_error(so, "Encountered 0 sensors or 0 neighbors in the cache file.");
		goto exit_bad_file;
	}
	if (sensors_count > (uint64_t)SKIN_SENSOR_MAX + 1)
	{
		skin_log_set_error(so, "Too many sensors in the cache file.  Enable \"Large skin\" mode in configuration");
		goto exit_bad_file;
	}

	so->p_sensors_count			= sensors_count;
	so->p_sensor_neighbors_count		= neighbors_count;
	so->p_sensor_neighbors			= malloc(so->p_sensor_neighbors_count * sizeof(*so->p_sensor_neighbors));
	so->p_sensors				= malloc(so->p_sensors_count * sizeof(*so->p_sensors));
	if (!so->p_sensor_neighbors || !so->p_sensors)
	{
		skin_log_set_error(so, "Could not acquire memory for main data structures");
		goto exit_no_mem;
	}

	cur = 0;
	for (si = 0; si < so->p_sensors_count; ++si)
	{
		unsigned long robot_link, neighbors_count;

		fscanf(cch, "%f %f %f %f %f %f %f %f %f %lu %lu",
				&so->p_sensors[si].relative_position[0],
				&so->p_sensors[si].relative_position[1],
				&so->p_sensors[si].relative_position[2],
				&so->p_sensors[si].relative_orientation[0],
				&so->p_sensors[si].relative_orientation[1],
				&so->p_sensors[si].relative_orientation[2],
				&so->p_sensors[si].flattened_position[0],
				&so->p_sensors[si].flattened_position[1],
				&so->p_sensors[si].radius,
				&robot_link, &neighbors_count);
		so->p_sensors[si].robot_link = robot_link;
		so->p_sensors[si].neighbors_count = neighbors_count;
		SKIN_CHECK_FILE_INTEGRITY(so, cch, "calibration cache");

		so->p_sensors[si].neighbors = so->p_sensor_neighbors + cur;
		for (sj = 0; sj < so->p_sensors[si].neighbors_count; ++sj)
		{
			unsigned long id;

			fscanf(cch, "%lu", &id);
			so->p_sensors[si].neighbors[sj] = id;
		}
		cur += so->p_sensors[si].neighbors_count;
	}
	SKIN_CHECK_FILE_INTEGRITY(so, cch, "calibration cache");

	ret = _calibrate_kernel(so);
	if (ret < 0)
		return ret;
	skin_log("Calibrating skin from cache... done");

	return ret;
exit_no_mem:
	ret = SKIN_NO_MEM;
	goto exit_fail;
exit_bad_file:
	ret = SKIN_FILE_INVALID;
exit_fail:
	skin_object_free(so);
	return ret;
}

int skin_object_regionalization_begin(skin_object *so)
{
	return _load_common(so, SKIN_LOAD_FOR_REGIONALIZATION);
}

/* some constructs internal to regionalization_end: */
struct regions_of_module
{
	skin_module_id			mid;
	skin_region_id			*regs;			/* sorted set of regions this module belongs to (unique values) */
	uint32_t			size;
	uint32_t			mem_size;
};

static bool _regions_equal(const struct regions_of_module *r1, const struct regions_of_module *r2)
{
	skin_region_id *regs1 = r1->regs;
	skin_region_id *regs2 = r2->regs;
	uint32_t size1 = r1->size;
	uint32_t size2 = r2->size;
	uint32_t i;

	if (size1 != size2)
		return false;
	for (i = 0; i < size1; ++i)
		if (regs1[i] != regs2[i])
			return false;
	return true;
}

static int _regions_of_module_cmp(const void *a, const void *b)
{
	const struct regions_of_module *first = a;
	const struct regions_of_module *second = b;
	skin_region_id *regs1 = first->regs;
	skin_region_id *regs2 = second->regs;
	uint32_t size1 = first->size;
	uint32_t size2 = second->size;
	uint32_t min_size;
	uint32_t i;

	/* check the regs set and compare according to that */
	min_size = size1 < size2?size1:size2;
	for (i = 0; i < min_size; ++i)
	{
		if (regs1[i] < regs2[i])
			return -1;
		if (regs1[i] > regs2[i])
			return 1;
	}
	if (size2 > min_size)
		return -1;
	if (size1 > min_size)
		return 1;

	/* sort according to mid doesn't really matter, but makes the cache file look nicer */
	if (first->mid < second->mid)
		return -1;
	if (first->mid > second->mid)
		return 1;
	return 0;
}

static int _regions_of_module_add_region(struct regions_of_module *rom, skin_region_id r)
{
	/*
	 * because of the order in which this function is called, regions will end up
	 * being sorted, so just check for uniqueness
	 */
	if (rom->size > 0 && rom->regs[rom->size - 1] == r)
		return 0;
#if SKIN_DEBUG
	if (rom->size > 0 && rom->regs[rom->size - 1] > r)
		skin_log("Internal warning: regions were supposed to be added to module_to_regions map in order");
#endif

	if (rom->size >= rom->mem_size)
	{
		skin_region_id *enlarged;
		uint32_t new_size;

		new_size = rom->mem_size * 2;
		if (new_size < 16)
			new_size = 16;
		enlarged = realloc(rom->regs, new_size * sizeof(*enlarged));
		if (enlarged == NULL)
			return -1;
		rom->regs = enlarged;
		rom->mem_size = new_size;
	}

	rom->regs[rom->size++] = r;
	return 0;
}

struct sub_region_data
{
	skin_sensor_id			*sensors_begin;		/* size: sensor types */
	skin_sensor_id			*sensors_end;		/* size: sensor types */
/*	vector<skin_sensor_id>		sensors_begin,
					sensors_end;*/
	skin_sub_region_id		sub_region_id;
	bool				first_encounter;
};

struct sub_region_indices
{
	skin_sub_region_id		*indices;
	skin_sub_region_index_size	size;
	skin_sub_region_index_size	mem_size;
};

static int _add_sub_region_id(struct sub_region_indices *data, skin_sub_region_id sr)
{
	if (data->size >= data->mem_size)
	{
		skin_sub_region_id *enlarged;
		skin_sub_region_index_size new_size;

		new_size = data->mem_size * 2;
		if (new_size < 16)
			new_size = 16;
		enlarged = realloc(data->indices, new_size * sizeof(*enlarged));
		if (enlarged == NULL)
			return -1;
		data->indices = enlarged;
		data->mem_size = new_size;
	}

	data->indices[data->size++] = sr;
	return 0;
}

struct regionalization_data
{
	struct sub_region_data		*sub_regions;
	uint64_t			size;			/* size of sub_regions */
	uint64_t			mem_size;
	struct sub_region_indices	*sub_region_indices;	/* for each region, indices of its sub-regions */
								/* size of this array is regions_count */
};

static int _add_sub_region(struct regionalization_data *data, struct sub_region_data *sr)
{
	if (data->size >= data->mem_size)
	{
		struct sub_region_data *enlarged;
		uint64_t new_size;

		new_size = data->mem_size * 2;
		if (new_size < 256)
			new_size = 256;
		enlarged = realloc(data->sub_regions, new_size * sizeof(*enlarged));
		if (enlarged == NULL)
			return -1;
		data->sub_regions = enlarged;
		data->mem_size = new_size;
	}

	data->sub_regions[data->size++] = *sr;
	return 0;
}

static int _reset_sub_region(struct sub_region_data *sr, skin_sensor_id *next_free_sensors,
		skin_sensor_type_size sensor_types_count, struct regionalization_data *data)
{
	skin_sensor_type_id i;

	sr->sensors_begin = malloc(sensor_types_count * sizeof(*sr->sensors_begin));
	sr->sensors_end = malloc(sensor_types_count * sizeof(*sr->sensors_end));
	if (!sr->sensors_begin || !sr->sensors_end)
		return -1;
	sr->sub_region_id = data->size;
	for (i = 0; i < sensor_types_count; ++i)
		sr->sensors_begin[i] = next_free_sensors[i];
	sr->first_encounter = true;
	return 0;
}

static int _set_module_mapping(skin_object *so, struct regions_of_module *cur, struct sub_region_data *sr,
		skin_sensor_id *next_free_sensors, struct regionalization_data *data)
{
	skin_module *m;
	skin_module_id i;
	uint32_t j;

	m = &so->p_modules[cur->mid];
	for (i = m->sensors_begin; i < m->sensors_end; ++i)
		so->p_sensors[i].id = next_free_sensors[m->sensor_type_id]++;

	if (sr->first_encounter)
		for (j = 0; j < cur->size; ++j)
			if (_add_sub_region_id(&data->sub_region_indices[cur->regs[j]], sr->sub_region_id))
				return -1;
	sr->first_encounter = false;
	return 0;
}

static int _finalize_sub_region(struct sub_region_data *sr, skin_sensor_id *next_free_sensors,
		skin_sensor_type_size sensor_types_count, struct regionalization_data *data)
{
	skin_sensor_type_id i;

	for (i = 0; i < sensor_types_count; ++i)
		sr->sensors_end[i] = next_free_sensors[i];
	return _add_sub_region(data, sr);
}

static int _calculate_sub_regions_and_sub_region_indices(skin_object *so,
		struct regionalization_data *reg_data, skin_regionalization_data *regions,
		skin_region_size *regs_count)
{
	struct regions_of_module *module_to_regions = NULL;
	struct sub_region_data sub_region;
	struct regions_of_module *last_regs = NULL;
	skin_sensor_id *next_free_sensor = NULL;
	skin_region_size regions_count = *regs_count;
	skin_module_size modules_count = so->p_modules_count;
	skin_sensor_type_size sensor_types_count = so->p_sensor_types_count;
	skin_module_id mi;
	skin_region_id ri;
	bool first_time = true;
	uint32_t i;
	int ret;

	*reg_data = (struct regionalization_data){0};
	/* create a map from modules to regions they belong to */
	module_to_regions = malloc(modules_count * sizeof(*module_to_regions));
	if (!module_to_regions)
		goto exit_no_mem;
	for (mi = 0; mi < modules_count; ++mi)
		module_to_regions[mi] = (struct regions_of_module){0};
	for (ri = 0; ri < regions_count; ++ri)
	{
		skin_regionalization_data *cur = &regions[ri];
		for (i = 0; i < cur->size; ++i)
		{
			skin_module_id mid = cur->modules[i];

			if (mid >= modules_count)
			{
				skin_log_set_error(so, "Ignoring invalid module id %u.  There are only %u modules in the skin",
						mid, modules_count);
				continue;
			}
			module_to_regions[mid].mid = mid;
			if (_regions_of_module_add_region(&module_to_regions[mid], ri))
				goto exit_no_mem;
		}
	}

	/*
	 * go over the module map and make sure all modules are included.
	 * If not, put them all in another region, with index regions_count
	 */
	first_time = true;
	for (mi = 0; mi < modules_count; ++mi)
		if (module_to_regions[mi].size == 0)
		{
			if (first_time)
			{
				skin_log_set_error(so, "There are modules not included in any region.  Creating an extra region for them.");
				skin_log_no_newline("\tMissing modules are:");
			}
			first_time = false;
			skin_log_no_newline(" %u", mi);

			module_to_regions[mi].mid = mi;
			if (_regions_of_module_add_region(&module_to_regions[mi], regions_count))
				goto exit_no_mem;
		}
	if (!first_time)
	{
		skin_log("%s", "");	/* to print the ending new line.  %s is a workaround for gcc warning */
		++regions_count;
		*regs_count = regions_count;
	}

	/* sort the map so that modules belonging to the same set of regions become consecutive */
	qsort(module_to_regions, modules_count, sizeof(*module_to_regions), _regions_of_module_cmp);

	/*
	 * go over this map and group the modules that belong to the same set of regions in sub-regions.
	 * In this process, change the id of sensors of every visited module to the index they will
	 * move to (effectively creating a mapping)
	 */
	reg_data->sub_region_indices = malloc(regions_count * sizeof(*reg_data->sub_region_indices));
	if (reg_data->sub_region_indices == NULL)
		goto exit_no_mem;
	for (ri = 0; ri < regions_count; ++ri)
		reg_data->sub_region_indices[ri] = (struct sub_region_indices){0};

	next_free_sensor = malloc(sensor_types_count * sizeof(*next_free_sensor));
	if (next_free_sensor == NULL)
		goto exit_no_mem;
	memset(next_free_sensor, 0, sensor_types_count * sizeof(*next_free_sensor));

	for (mi = 0; mi < modules_count; ++mi)
	{
		if (last_regs == NULL || !_regions_equal(last_regs, &module_to_regions[mi]))
		{
			if (last_regs != NULL)
				/* finished with this sub-region */
				if (_finalize_sub_region(&sub_region, next_free_sensor, sensor_types_count, reg_data))
					goto exit_no_mem;
			if (_reset_sub_region(&sub_region, next_free_sensor, sensor_types_count, reg_data))
				goto exit_no_mem;
			last_regs = &module_to_regions[mi];
		}
		if (_set_module_mapping(so, &module_to_regions[mi], &sub_region, next_free_sensor, reg_data))
			goto exit_no_mem;
	}
	/* don't forget the last sub-region */
	if (last_regs != NULL)
		if (_finalize_sub_region(&sub_region, next_free_sensor, sensor_types_count, reg_data))
			goto exit_no_mem;

	if (reg_data->size > (uint64_t)SKIN_SUB_REGION_MAX + 1)
	{
		skin_log_set_error(so, "The current version of Skinware doesn't support more than %u sub-regions.\n"
				"The number of sub-regions increase if many regions overlap.\n"
				"If so many sub-regions are indeed desired, enable \"Large skin\" mode in configuration.", SKIN_SUB_REGION_MAX);
		ret = SKIN_FAIL;
		goto exit_fail;
	}

	/* done!  Now it's possible to cache and regionalize the kernel */
	ret = SKIN_SUCCESS;
exit_cleanup:
	if (module_to_regions)
		for (mi = 0; mi < modules_count; ++mi)
			free(module_to_regions[mi].regs);
	free(module_to_regions);
	free(next_free_sensor);
	return ret;
exit_no_mem:
	ret = SKIN_NO_MEM;
exit_fail:
	/* cleanup would be done by caller anyway */
	goto exit_cleanup;
}

int skin_object_regionalization_end(skin_object *so, skin_regionalization_data *regions,
		skin_region_size regions_count, const char *cache)
{
	struct regionalization_data reg_data;
	skin_sub_region_index_size total_indices;
	skin_region_id ri;
	skin_sensor_type_id sti;
	uint64_t i;
	skin_sub_region_index_id cur_sub_region_index;
	int ret = SKIN_FAIL;

	skin_reader_internal_terminate(&so->p_reader);

	/*
	 * if count is 0, then they would all be added to the extra region covering
	 * missing modules, so no error is needed to be given.
	 *
	 * There could be an extra region created for uncovered modules,
	 * so instead of usual check of `count > MAX + 1`, we check for `count > MAX`
	 */
	if (regions_count > (uint64_t)SKIN_REGION_MAX)
	{
		skin_log_set_error(so, "The current version of Skinware doesn't support more than %u regions.\n"
				"If so many regions are indeed desired, enable \"Large skin\" mode in configuration.", SKIN_REGION_MAX);
		return SKIN_FAIL;
	}

	/* calculate sub-region stuff */
	if (_calculate_sub_regions_and_sub_region_indices(so, &reg_data, regions, &regions_count))
		goto exit_no_mem;

	/* cache the results */
	total_indices = 0;
	for (ri = 0; ri < regions_count; ++ri)
		total_indices += reg_data.sub_region_indices[ri].size;

	if (cache)
	{
		FILE *cch;
		skin_sensor_id si;

		cch = fopen(cache, "w");
		if (cch == NULL)
		{
			skin_log_set_error(so, "Could not open cache file: %s for writing.  Results will not be cached.", cache);
			goto exit_regionalize;
		}

		fprintf(cch, "%u %u\n", so->p_sensors_count, so->p_sensor_types_count);
		for (sti = 0; sti < so->p_sensor_types_count; ++sti)
		{
			fprintf(cch, "%u", so->p_sensor_types[sti].sensors_end - so->p_sensor_types[sti].sensors_begin);
			for (si = so->p_sensor_types[sti].sensors_begin; si < so->p_sensor_types[sti].sensors_end; ++si)
				fprintf(cch, " %u", so->p_sensors[si].id);
			fprintf(cch, "\n");
		}
		fprintf(cch, "\n%llu\n", (unsigned long long)reg_data.size);
		for (i = 0; i < reg_data.size; ++i)
		{
			for (sti = 0; sti < so->p_sensor_types_count; ++sti)
				fprintf(cch, "%s%5u %5u", sti?" ":"", reg_data.sub_regions[i].sensors_begin[sti],
						reg_data.sub_regions[i].sensors_end[sti]);
			fprintf(cch, "\n");
		}
		/* total_indices is there for regionalization_reload to preallocate memory */
		fprintf(cch, "\n%u %llu\n", regions_count, (unsigned long long)total_indices);
		for (ri = 0; ri < regions_count; ++ri)
		{
			fprintf(cch, "%llu", (unsigned long long)reg_data.sub_region_indices[ri].size);
			for (i = 0; i < reg_data.sub_region_indices[ri].size; ++i)
				fprintf(cch, " %u", reg_data.sub_region_indices[ri].indices[i]);
			fprintf(cch, "\n");
		}

		/* print the format if user wants to modify the cache manually */
		fprintf(cch, "\nFormat:\n"
			"sensors-count  sensor-types-count\n"
			"(sensor-types-count lines) sensors-of-type-count  sensor-id (sensors-of-type-count numbers, "
				"for the sensor map, relative to the sensor type)\n"
			"sub-regions-count\n"
			"(sub-regions-count lines) sensors-begin  sensors-end (sensor-types-count pairs)\n"
			"regions-count  total-sub-region-indices-count\n"
			"(regions-count lines) sub-regions-of-region-count  sub-region (sub-regions-of-region-count numbers)\n");
		fclose(cch);
	}

exit_regionalize:
	so->p_sub_regions_count			= reg_data.size;
	so->p_regions_count			= regions_count;
	so->p_sub_region_sensors_begins_count	= so->p_sensor_types_count * (uint32_t)so->p_sub_regions_count;
	so->p_sub_region_sensors_ends_count	= so->p_sensor_types_count * (uint32_t)so->p_sub_regions_count;
	so->p_region_indices_count		= total_indices;
	so->p_sub_region_indices_count		= total_indices;
	so->p_sub_region_sensors_begins		= malloc(so->p_sub_region_sensors_begins_count * sizeof(*so->p_sub_region_sensors_begins));
	so->p_sub_region_sensors_ends		= malloc(so->p_sub_region_sensors_ends_count * sizeof(*so->p_sub_region_sensors_ends));
	so->p_sub_region_indices		= malloc(so->p_sub_region_indices_count * sizeof(*so->p_sub_region_indices));
	so->p_regions				= malloc(so->p_regions_count * sizeof(*so->p_regions));
	if (!so->p_sub_region_indices || !so->p_sub_region_sensors_begins || !so->p_sub_region_sensors_ends || !so->p_regions)
	{
		skin_log_set_error(so, "Could not acquire memory for regionalization");
		goto exit_no_mem;
	}
	for (i = 0; i < reg_data.size; ++i)
	{
		for (sti = 0; sti < so->p_sensor_types_count; ++sti)
		{
			so->p_sub_region_sensors_begins[i * (uint32_t)so->p_sensor_types_count + sti] = reg_data.sub_regions[i].sensors_begin[sti];
			so->p_sub_region_sensors_ends[i * (uint32_t)so->p_sensor_types_count + sti] = reg_data.sub_regions[i].sensors_end[sti];
		}
	}
	cur_sub_region_index = 0;
	for (ri = 0; ri < regions_count; ++ri)
	{
		skin_sub_region_index_id srii;

		so->p_regions[ri].sub_region_indices_begin = cur_sub_region_index;
		for (srii = 0; srii < reg_data.sub_region_indices[ri].size; ++srii)
			so->p_sub_region_indices[cur_sub_region_index++] = reg_data.sub_region_indices[ri].indices[srii];
		so->p_regions[ri].sub_region_indices_end = cur_sub_region_index;
	}

	ret = _regionalize_kernel(so);
exit_cleanup:
	if (reg_data.sub_regions)
		for (i = 0; i < reg_data.size; ++i)
		{
			free(reg_data.sub_regions[i].sensors_begin);
			free(reg_data.sub_regions[i].sensors_end);
		}
	if (reg_data.sub_region_indices)
		for (ri = 0; ri < regions_count; ++ri)
			free(reg_data.sub_region_indices[ri].indices);
	free(reg_data.sub_regions);
	free(reg_data.sub_region_indices);
	return ret;
exit_no_mem:
	ret = SKIN_NO_MEM;
	skin_object_free(so);
	goto exit_cleanup;
}

int skin_object_regionalization_reload(skin_object *so, const char *cache)
{
	FILE *cch;
	unsigned long long sensors_count, types_count;
	unsigned long long sub_regions_count, regions_count;
	unsigned long long sub_region_indices_count;
	skin_sensor_type_id sti;
	skin_sub_region_id sri;
	skin_region_id ri;
	skin_sensor_id cur_sensor;
	skin_sub_region_index_id cur_sub_region_index;
	int ret;

	if (!cache)
		return SKIN_FAIL;

	cch = fopen(cache, "r");
	/*
	 * could be that so is called just to check if the cache exists.
	 * Therefore if it doesn't exist, don't log error
	 */
	if (cch == NULL)
		return SKIN_NO_FILE;

	skin_log("Regionalizing skin from cache...");
	fscanf(cch, "%llu %llu", &sensors_count, &types_count);
	SKIN_CHECK_FILE_INTEGRITY(so, cch, "regionalization cache");
	if (sensors_count == 0 || types_count == 0)
	{
		skin_log_set_error(so, "Encountered 0 sensors or 0 sensor types in the cache file.");
		goto exit_bad_file;
	}
	if (sensors_count > (uint64_t)SKIN_SENSOR_MAX + 1 || types_count > (uint64_t)SKIN_SENSOR_TYPE_MAX + 1)
	{
		skin_log_set_error(so, "Too many sensors or sensor types in the cache file.  Enable \"Large skin\" mode in configuration");
		goto exit_bad_file;
	}

	so->p_sensors_count		= sensors_count;
	so->p_sensor_types_count	= types_count;
	so->p_sensor_types		= malloc(so->p_sensor_types_count * sizeof(*so->p_sensor_types));
	so->p_sensors			= malloc(so->p_sensors_count * sizeof(*so->p_sensors));
	if (!so->p_sensor_types || !so->p_sensors)
	{
		skin_log_set_error(so, "Could not acquire memory for main data structures");
		goto exit_no_mem;
	}

	cur_sensor = 0;
	for (sti = 0; sti < so->p_sensor_types_count; ++sti)
	{
		unsigned long sensor_count = 0;
		skin_sensor_id si;

		so->p_sensor_types[sti].sensors_begin = cur_sensor;
		fscanf(cch, "%lu", &sensor_count);
		SKIN_CHECK_FILE_INTEGRITY(so, cch, "regionalization cache");
		for (si = 0; si < sensor_count; ++si)
		{
			unsigned long id;

			fscanf(cch, "%lu", &id);
			so->p_sensors[cur_sensor++].id = id;
		}
		so->p_sensor_types[sti].sensors_end = cur_sensor;
	}

	fscanf(cch, "%llu", &sub_regions_count);
	SKIN_CHECK_FILE_INTEGRITY(so, cch, "regionalization cache");
	if (sub_regions_count == 0)
	{
		skin_log_set_error(so, "Encountered 0 sub-regions in the cache file.");
		goto exit_bad_file;
	}
	if (sub_regions_count > (uint64_t)SKIN_SUB_REGION_MAX + 1)
	{
		skin_log_set_error(so, "Too many sub-regions in the cache file.  Enable \"Large skin\" mode in configuration");
		goto exit_bad_file;
	}

	so->p_sub_regions_count		= sub_regions_count;
	so->p_sub_regions		= malloc(so->p_sub_regions_count * sizeof(*so->p_sub_regions));
	so->p_sub_region_sensors_begins	= malloc(so->p_sub_regions_count * (uint32_t)so->p_sensor_types_count * sizeof(*so->p_sub_region_sensors_begins));
	so->p_sub_region_sensors_ends	= malloc(so->p_sub_regions_count * (uint32_t)so->p_sensor_types_count * sizeof(*so->p_sub_region_sensors_ends));
	if (!so->p_sub_regions || !so->p_sub_region_sensors_begins || !so->p_sub_region_sensors_ends)
	{
		skin_log_set_error(so, "Could not acquire memory for main data structures");
		goto exit_no_mem;
	}

	for (sri = 0; sri < so->p_sub_regions_count; ++sri)
		for (sti = 0; sti < so->p_sensor_types_count; ++sti)
		{
			unsigned long sensors_begin, sensors_end;

			fscanf(cch, "%lu %lu", &sensors_begin, &sensors_end);
			so->p_sub_region_sensors_begins[sri * (uint32_t)so->p_sensor_types_count + sti] = sensors_begin;
			so->p_sub_region_sensors_ends[sri * (uint32_t)so->p_sensor_types_count + sti] = sensors_end;
		}

	fscanf(cch, "%llu %llu", &regions_count, &sub_region_indices_count);
	SKIN_CHECK_FILE_INTEGRITY(so, cch, "regionalization cache");
	if (regions_count == 0 || sub_region_indices_count == 0)
	{
		skin_log_set_error(so, "Encountered 0 regions or 0 sub-region indices in the cache file.");
		goto exit_bad_file;
	}
	if (regions_count > (uint64_t)SKIN_REGION_MAX+1)
	{
		skin_log_set_error(so, "Too many regions in the cache file.  Enable \"Large skin\" mode in configuration");
		goto exit_bad_file;
	}

	so->p_regions_count		= regions_count;
	so->p_sub_region_indices_count	= sub_region_indices_count;
	so->p_region_indices_count	= so->p_sub_region_indices_count;
	so->p_sub_region_indices	= malloc(so->p_sub_region_indices_count * sizeof(*so->p_sub_region_indices));
	so->p_regions			= malloc(so->p_regions_count * sizeof(*so->p_regions));
	if (!so->p_sub_region_indices || !so->p_regions)
	{
		skin_log_set_error(so, "Could not acquire memory for main data structures");
		goto exit_no_mem;
	}

	cur_sub_region_index = 0;
	for (ri = 0; ri < so->p_regions_count; ++ri)
	{
		unsigned long long sub_region_indices_count = 0;
		skin_sub_region_index_id srii;

		so->p_regions[ri].sub_region_indices_begin = cur_sub_region_index;
		fscanf(cch, "%llu", &sub_region_indices_count);
		SKIN_CHECK_FILE_INTEGRITY(so, cch, "regionalization cache");
		for (srii = 0; srii < sub_region_indices_count; ++srii)
		{
			unsigned long id;

			fscanf(cch, "%lu", &id);
			so->p_sub_region_indices[cur_sub_region_index++] = id;
		}
		so->p_regions[ri].sub_region_indices_end = cur_sub_region_index;
	}
	SKIN_CHECK_FILE_INTEGRITY(so, cch, "regionalization cache");

	ret = _regionalize_kernel(so);
	if (ret < 0)
		return ret;
	skin_log("Regionalizing skin from cache... done");

	return ret;
exit_no_mem:
	ret = SKIN_NO_MEM;
	goto exit_fail;
exit_bad_file:
	ret = SKIN_FILE_INVALID;
exit_fail:
	skin_object_free(so);
	return ret;
}
#endif /* !SKIN_DS_ONLY */

void skin_object_for_each_sensor(skin_object *so, skin_callback_sensor c, void *data)
{
	skin_sensor_id i;
	skin_sensor_id begin, end;
	skin_sensor *sensors;

	sensors = so->p_sensors;

	/*
	 * the sensors are ordered by sensor type already, so just calling them one by one
	 * satisfies their sensor-type-ordering requirement
	 */
	begin = 0;
	end = so->p_sensors_count;
	for (i = begin; i < end; ++i)
		if (c(&sensors[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_object_for_each_sensor_type(skin_object *so, skin_callback_sensor_type c, void *data)
{
	skin_sensor_type_id i;
	skin_sensor_type_id begin, end;
	skin_sensor_type *sensor_types;

	sensor_types = so->p_sensor_types;

	begin = 0;
	end = so->p_sensor_types_count;
	for (i = begin; i < end; ++i)
		if (c(&sensor_types[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_object_for_each_sub_region(skin_object *so, skin_callback_sub_region c, void *data)
{
	skin_sub_region_id i;
	skin_sub_region_id begin, end;
	skin_sub_region *sub_regions;

	sub_regions = so->p_sub_regions;

	begin = 0;
	end = so->p_sub_regions_count;
	for (i = begin; i < end; ++i)
		if (c(&sub_regions[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_object_for_each_region(skin_object *so, skin_callback_region c, void *data)
{
	skin_region_id i;
	skin_region_id begin, end;
	skin_region *regions;

	regions = so->p_regions;

	begin = 0;
	end = so->p_regions_count;
	for (i = begin; i < end; ++i)
		if (c(&regions[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_object_for_each_module(skin_object *so, skin_callback_module c, void *data)
{
	skin_module_id i;
	skin_module_id begin, end;
	skin_module *modules;

	modules = so->p_modules;

	begin = 0;
	end = so->p_modules_count;
	for (i = begin; i < end; ++i)
		if (c(&modules[i], data) == SKIN_CALLBACK_STOP)
			return;
}

void skin_object_for_each_patch(skin_object *so, skin_callback_patch c, void *data)
{
	skin_patch_id i;
	skin_patch_id begin, end;
	skin_patch *patches;

	patches = so->p_patches;

	begin = 0;
	end = so->p_patches_count;
	for (i = begin; i < end; ++i)
		if (c(&patches[i], data) == SKIN_CALLBACK_STOP)
			return;
}
