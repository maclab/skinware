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

#ifndef SKIN_COMMON_H
#define SKIN_COMMON_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_kernel.h"
#include "skin_compiler.h"
#include "skin_version.h"

#define SKIN_SUCCESS			0		/* returned upon success */
#define SKIN_PARTIAL			1		/* returned only if the operation was partially successful */
#define SKIN_FAIL			-1		/* if operation failed */
#define SKIN_NO_FILE			-2		/* if a file could not be opened */
#define SKIN_NO_MEM			-3		/* if a memory could not be allocated */
#define SKIN_FILE_INCOMPLETE		-4		/* if unexpected eof encountered */
#define SKIN_FILE_PARSE_ERROR		-5		/* if file was erroneous */
#define SKIN_FILE_INVALID		-6		/* if file data does not match kernel data */
#define SKIN_LOCK_TIMEOUT		-7		/* if lock timedout */
#define SKIN_LOCK_NOT_ACQUIRED		-8		/* if lock was not acquired (in a try lock) */
#define SKIN_BAD_DATA			-9		/* if bad data given to service creation */
#define SKIN_BAD_NAME			-10		/* if bad name given to service creation */
#define SKIN_BAD_ID			-11		/* if bad id given to indicate a service or a layer */

#define SKIN_INVALID_ID			SKINK_INVALID_ID
#define SKIN_INVALID_SIZE		SKINK_INVALID_SIZE

typedef skink_misc_datum		skin_kernel_misc_data;
typedef skink_sensor_layer_id		skin_sensor_type_id;
typedef skink_sensor_id			skin_sensor_id;
typedef skink_sub_region_id		skin_sub_region_id;
typedef skink_region_id			skin_region_id;
typedef skink_module_id			skin_module_id;
typedef skink_patch_id			skin_patch_id;
typedef skink_region_index_id		skin_region_index_id;
typedef skink_sub_region_index_id	skin_sub_region_index_id;

typedef skink_sensor_layer_size		skin_sensor_type_size;
typedef skink_sensor_size		skin_sensor_size;
typedef skink_sub_region_size		skin_sub_region_size;
typedef skink_region_size		skin_region_size;
typedef skink_module_size		skin_module_size;
typedef skink_patch_size		skin_patch_size;
typedef skink_region_index_size		skin_region_index_size;
typedef skink_sub_region_index_size	skin_sub_region_index_size;

#define SKIN_SENSOR_TYPE_MAX		SKINK_SENSOR_LAYER_MAX
#define SKIN_SENSOR_MAX			SKINK_SENSOR_MAX
#define SKIN_SUB_REGION_MAX		SKINK_SUB_REGION_MAX
#define SKIN_REGION_MAX			SKINK_REGION_MAX
#define SKIN_MODULE_MAX			SKINK_MODULE_MAX
#define SKIN_PATCH_MAX			SKINK_PATCH_MAX

typedef skink_sensor_response		skin_sensor_response;
#define SKIN_SENSOR_RESPONSE_MAX	SKINK_SENSOR_RESPONSE_MAX

#endif
