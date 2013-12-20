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

#ifndef SKINK_COMMON_H
#define SKINK_COMMON_H

#include "skink_config.h"

#ifdef __KERNEL__
# include <linux/types.h>
#else
// C99 says __STDC_LIMIT_MACROS and __STDC_CONSTANT_MACROS need to be defined before including stdint.h to have
// access to min/max macros (such as UINT8_MAX) as well as suffix macros (such as INT32_C)
# ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS 1
# endif
# ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS 1
# endif
# include <stdint.h>
#endif

#include "skink_version.h"

#ifdef __KERNEL__
#define 				SKINK_SUCCESS					0	// when operation was successful
#define					SKINK_PARTIAL					1	// if operation was partially successful
#define 				SKINK_FAIL					-1	// if operation failed
#define					SKINK_TOO_EARLY					-2	// too early to perform this operation (must wait and retry)
#define					SKINK_IN_USE					-3	// device is in use, try with another id
#define					SKINK_TOO_LATE					-4	// too late for this operation (must give up!)
#define					SKINK_BAD_ID					-5	// provided id is not valid
#define					SKINK_BAD_NAME					-6	// provided name is not valid (for example shared memory name)
#define					SKINK_BAD_DATA					-7	// bad input data to the function
#define					SKINK_BAD_CONTEXT				-8	// if function is called in bad context (for example needs real-time context)
#define					SKINK_NO_MEM					-9	// not enough memory
#endif

#define					SKINK_INVALID_ID				0xffffffffu
#define					SKINK_INVALID_SIZE				SKINK_INVALID_ID

// Miscellaneous
typedef					uint32_t					skink_misc_datum;

// ID types
#if SKINK_LARGE_SKIN
typedef					uint32_t					skink_sensor_layer_id;
typedef					uint32_t					skink_sensor_id;
typedef					uint32_t					skink_sub_region_id;
typedef					uint32_t					skink_region_id;
typedef					uint32_t					skink_module_id;
typedef					uint32_t					skink_patch_id;
typedef					uint64_t					skink_region_index_id;
typedef					uint64_t					skink_sub_region_index_id;
#define					SKINK_SENSOR_LAYER_MAX				((skink_sensor_layer_size)0xfffffffe)
#define					SKINK_SENSOR_MAX				((skink_sensor_size)0xfffffffe)
#define					SKINK_SUB_REGION_MAX				((skink_sub_region_size)0xfffffffe)
#define					SKINK_REGION_MAX				((skink_region_size)0xfffffffe)
#define					SKINK_MODULE_MAX				((skink_module_size)0xfffffffe)
#define					SKINK_PATCH_MAX					((skink_patch_size)0xfffffffe)
// Note: When checking to see if adding more sensors/modules/etc gets above SKIN_*_MAX, make sure you cast them all to uint64_t to avoid overflow
#else
typedef					uint16_t					skink_sensor_layer_id;
typedef					uint16_t					skink_sensor_id;
typedef					uint16_t					skink_sub_region_id;
typedef					uint16_t					skink_region_id;
typedef					uint16_t					skink_module_id;
typedef					uint16_t					skink_patch_id;
typedef					uint32_t					skink_region_index_id;
typedef					uint32_t					skink_sub_region_index_id;
#define					SKINK_SENSOR_LAYER_MAX				((skink_sensor_layer_size)0xfffe)
#define					SKINK_SENSOR_MAX				((skink_sensor_size)0xfffe)
#define					SKINK_SUB_REGION_MAX				((skink_sub_region_size)0xfffe)
#define					SKINK_REGION_MAX				((skink_region_size)0xfffe)
#define					SKINK_MODULE_MAX				((skink_module_size)0xfffe)
#define					SKINK_PATCH_MAX					((skink_patch_size)0xfffe)
// Note: When checking to see if adding more sensors/modules/etc gets above SKIN_*_MAX, make sure you cast them all to uint64_t (or uint32_t) to avoid overflow
#endif

// Size types
typedef					skink_sensor_layer_id				skink_sensor_layer_size;
typedef					skink_sensor_id					skink_sensor_size;
typedef					skink_sub_region_id				skink_sub_region_size;
typedef					skink_region_id					skink_region_size;
typedef					skink_module_id					skink_module_size;
typedef					skink_patch_id					skink_patch_size;
typedef					skink_region_index_id				skink_region_index_size;
typedef					skink_sub_region_index_id			skink_sub_region_index_size;

// Sensor responses
typedef					uint16_t					skink_sensor_response;
#define					SKINK_SENSOR_RESPONSE_MAX			((skink_sensor_response)0xffff)

#define					SKINK_COMMAND_PROCFS_ENTRY			"skink_command"

#ifdef __KERNEL__
  // The following part can only be used in the kernel module, and when this header file is included in the top-layer, it should be unavailable

# ifndef				FILE_LINE
#  define				FILE_LINE					"...%s("SKINK_STRINGIFY(__LINE__)"): "
# endif
# ifndef				END_OF_FILE_PATH
#  define				END_OF_FILE_PATH				__FILE__ + (sizeof(__FILE__) < 25?0:(sizeof(__FILE__) - 15))
# endif
# ifndef				SKINK_MODULE_NAME
#  warning Define SKINK_MODULE_NAME to a string before including skin kernel header files to have that as prefix to your logs when using SKINK_[RT_]LOG
#  define				SKINK_MODULE_NAME				"Unnamed Module"
# endif
# if					SKINK_DEBUG
#  define				SKINK_LOG(x, ...)				printk(KERN_INFO SKINK_MODULE_NAME": "FILE_LINE x"\n",\
													END_OF_FILE_PATH, ##__VA_ARGS__)
#  define				SKINK_DBG(x, ...)				SKINK_LOG(x, ##__VA_ARGS__)
# else
#  define				SKINK_LOG(x, ...)				printk(KERN_INFO SKINK_MODULE_NAME": "x"\n", ##__VA_ARGS__)
#  define				SKINK_DBG(x, ...)				((void)0)
# endif

#endif //__KERNEL__

#endif
