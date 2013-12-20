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

#ifndef SKINK_INTERNAL_H
#define SKINK_INTERNAL_H

#ifdef __KERNEL__

#include "skink_shared_memory_ids.h"

#define					SKINK_CONVERTED_WRITER_PRIORITY			(SKIN_RT_MAX_PRIORITY - SKINK_KERNEL_ACQ_PRIORITY * SKIN_RT_MORE_PRIORITY)
#define					SKINK_CONVERTED_READER_PRIORITY			(SKIN_RT_MAX_PRIORITY - SKINK_SERVICE_ACQ_PRIORITY * SKIN_RT_MORE_PRIORITY)
#define					SKINK_CONVERTED_SERVICE_PRIORITY		(SKIN_RT_MAX_PRIORITY - SKINK_SERVICE_PRIORITY * SKIN_RT_MORE_PRIORITY)

#define					SKINK_MODULE_LOADED				the_misc_data[SKINK_MODULE_LOADED_INDEX]
#define					SKINK_INITIALIZED				the_misc_data[SKINK_INITIALIZED_INDEX]
#define					SKINK_INITIALIZATION_FAILED			the_misc_data[SKINK_INITIALIZATION_FAILED_INDEX]
#define					SKINK_SENSOR_LAYERS_COUNT			the_misc_data[SKINK_SENSOR_LAYERS_COUNT_INDEX]
#define					SKINK_SENSORS_COUNT				the_misc_data[SKINK_SENSORS_COUNT_INDEX]
#define					SKINK_SUB_REGIONS_COUNT				the_misc_data[SKINK_SUB_REGIONS_COUNT_INDEX]
#define					SKINK_REGIONS_COUNT				the_misc_data[SKINK_REGIONS_COUNT_INDEX]
#define					SKINK_MODULES_COUNT				the_misc_data[SKINK_MODULES_COUNT_INDEX]
#define					SKINK_PATCHES_COUNT				the_misc_data[SKINK_PATCHES_COUNT_INDEX]
#define					SKINK_SUB_REGION_INDICES_COUNT			the_misc_data[SKINK_SUB_REGION_INDICES_COUNT_INDEX]
#define					SKINK_REGION_INDICES_COUNT			the_misc_data[SKINK_REGION_INDICES_COUNT_INDEX]
#define					SKINK_SENSOR_NEIGHBORS_COUNT			the_misc_data[SKINK_SENSOR_NEIGHBORS_COUNT_INDEX]
#define					SKINK_CALIBRATION_POSSIBLE			the_misc_data[SKINK_CALIBRATION_POSSIBLE_INDEX]
#define					SKINK_CALIBRATION_TOO_EARLY			the_misc_data[SKINK_CALIBRATION_TOO_EARLY_INDEX]
#define					SKINK_REGIONALIZATION_POSSIBLE			the_misc_data[SKINK_REGIONALIZATION_POSSIBLE_INDEX]
#define					SKINK_REGIONALIZATION_TOO_EARLY			the_misc_data[SKINK_REGIONALIZATION_TOO_EARLY_INDEX]
#define					SKINK_REGIONALIZATION_SYNC			the_misc_data[SKINK_REGIONALIZATION_SYNC_INDEX]

#define					SKINK_SIZEOF_SENSOR_LAYER			the_misc_data[SKINK_SIZEOF_SENSOR_LAYER_INDEX]
#define					SKINK_SIZEOF_SENSOR				the_misc_data[SKINK_SIZEOF_SENSOR_INDEX]
#define					SKINK_SIZEOF_MODULE				the_misc_data[SKINK_SIZEOF_MODULE_INDEX]
#define					SKINK_SIZEOF_PATCH				the_misc_data[SKINK_SIZEOF_PATCH_INDEX]
#define					SKINK_SIZEOF_SUB_REGION				the_misc_data[SKINK_SIZEOF_SUB_REGION_INDEX]
#define					SKINK_SIZEOF_REGION				the_misc_data[SKINK_SIZEOF_REGION_INDEX]

// The following enum, enumerates the states of the kernel.  The defined string after the variable is the output of skink_phase sysfs file in that state
typedef enum skink_state
{
					SKINK_STATE_INIT		  /* init */	= 0,	// the skin kernel has just loaded
					SKINK_STATE_READY_FOR_DEVICES	  /* devs */	= 1,	// basic initialization done and ready to accept devices
					SKINK_STATE_NO_MORE_DEVICES	  /* nodevs */	= 2,	// from this state on, no more devices can register
					SKINK_STATE_DEVICES_INTRODUCED	  /* nodevs */	= 3,	// the state where all registrations are complete,
												// but the structures are not built yet
					SKINK_STATE_STRUCTURES_BUILT	  /* nodevs */	= 4,	// the internal data structures are built, threads will be spawned
					SKINK_STATE_THREAD_SPAWNED	  /* nodevs */	= 5,	// threads have spawned.  This state equals to calibrating, however
												// it gives more meaning to check whether threads have spawned (check if
												// in states before or after this) than whether calibration is possible
					SKINK_STATE_CALIBRATING		  /* clbr */	= 6,	// during calibration
					SKINK_STATE_REGIONALIZING	  /* rgn */	= 7,	// during regionalization
					SKINK_STATE_REGIONALIZATION_DONE  /* preop */	= 8,	// a state after regionalization is declared to be over, yet the
												// skin kernel has things to do before becoming fully operational
					SKINK_STATE_OPERATIONAL		  /* op */	= 9,	// services and user space programs can now use the skin
					SKINK_STATE_EXITING		  /* exit */	= 10,	// the skin kernel is being removed from kernel
					SKINK_STATE_FAILED		  /* fail */	= -1	// there was an error in initialization
} skink_state;

#endif //__KERNEL__

#endif
